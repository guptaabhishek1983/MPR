#include "vtkTriMeshPlaneCutter.h"

#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"
#include "vtkTimerLog.h"
#include "vtkSmartPointer.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPlane.h"
#include "vtkPolyDataWriter.h"

#include <iostream>
#include <sstream>
#include <vector>

//------------------------------------------------------------------------
// Write out the surface mesh.
// Log times taken to initialize and to compute the cut etc
#define DEBUGGING 0

//------------------------------------------------------------------------
// Timer logs
#if DEBUGGING
#define TIMER_START( x ) \
  vtkSmartPointer< vtkTimerLog > x = vtkSmartPointer< vtkTimerLog >::New(); \
  x->StartTimer();
#else
  #define TIMER_START( x )
#endif

#if DEBUGGING
#define TIMER_STOP( x, msg ) \
  x->StopTimer(); \
  std::cout << msg << " took " << x->GetElapsedTime() << " s." << std::endl;
#else
  #define TIMER_STOP( x, msg )
#endif


//------------------------------------------------------------------------
vtkStandardNewMacro(vtkTriMeshPlaneCutter);

//------------------------------------------------------------------------
vtkTriMeshPlaneCutter::vtkTriMeshPlaneCutter()
{
  this->Origin[0] = this->Origin[1] = this->Origin[2] = VTK_DOUBLE_MIN;
  this->Normal[0] = this->Normal[1] = this->Normal[2] = VTK_DOUBLE_MIN;

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------
vtkTriMeshPlaneCutter::~vtkTriMeshPlaneCutter()
{
}

//------------------------------------------------------------------------
inline double DistanceToPlane(double origin[3], double normal[3], double p[3])
{
  return 
    (p[0]-origin[0])*normal[0] +
    (p[1]-origin[1])*normal[1] +
    (p[2]-origin[2])*normal[2];
}

#define RTSS_INTERSECTION_TOLERANCE 1e-12

//------------------------------------------------------------------------
inline bool EdgePlaneIntersect(
  double planeOrigin[3], double planeNormal[3],
  double edgePt1[3], double edgePt2[3],
  double intersectionPt[3])
{
  const double d1 = DistanceToPlane(planeOrigin, planeNormal, edgePt1);
  const double d2 = DistanceToPlane(planeOrigin, planeNormal, edgePt2);

  // Intersection only if the signed distance lies on either side of the plane
  if ((d1 < 0 && d2 > 0) || (d1 > 0 && d2 < 0))
    {
    const double perpendicularLength = (d1-d2);
    if (fabs(perpendicularLength) < RTSS_INTERSECTION_TOLERANCE)
      {
      // Do not get into divide by zero sort of issues
      return false;
      }
    const double t = d1/perpendicularLength;
    intersectionPt[0] = (1-t)*edgePt1[0] + t*edgePt2[0];
    intersectionPt[1] = (1-t)*edgePt1[1] + t*edgePt2[1];
    intersectionPt[2] = (1-t)*edgePt1[2] + t*edgePt2[2];   
    return true;
    }

  // no intersection.
  return false;
}

//------------------------------------------------------------------------
inline bool EdgePlaneIntersectAxisAligned(
  double planeOrigin[3], int axis,
  double edgePt1[3], double edgePt2[3],
  double intersectionPt[3])
{
  const double d1 = edgePt1[axis]-planeOrigin[axis];
  const double d2 = edgePt2[axis]-planeOrigin[axis];

  // Intersection only if the signed distance lies on either side of the plane
  if ((d1 < 0 && d2 > 0) || (d1 > 0 && d2 < 0))
    {
    const double perpendicularLength = (d1-d2);
    if (fabs(perpendicularLength) < RTSS_INTERSECTION_TOLERANCE)
      {
      // Do not get into divide by zero sort of issues
      return false;
      }
    const double t = d1/perpendicularLength;
    intersectionPt[0] = (1-t)*edgePt1[0] + t*edgePt2[0];
    intersectionPt[1] = (1-t)*edgePt1[1] + t*edgePt2[1];
    intersectionPt[2] = (1-t)*edgePt1[2] + t*edgePt2[2];   
    return true;
    }

  // no intersection.
  return false;
}

//------------------------------------------------------------------------
void vtkTriMeshPlaneCutter::SetCutFunction( vtkPlane *plane )
{
  this->SetNormal(plane->GetNormal());
  this->SetOrigin(plane->GetOrigin());
}

//------------------------------------------------------------------------
int vtkTriMeshPlaneCutter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input (the triangle mesh to be cut)
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // the cut that will be generated.
  vtkPolyData *cutPolyData = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // if the plane is axis aligned, set axis and use an even more
  // optimized method
  int axis = -1;
  for (int i = 0; i < 3; i++)
    {
    if (fabs(fabs(this->Normal[i]) - 1.0) < 1e-6)
      {
      axis = i;
      }
    }
  
  if (axis == -1)
    {
    // Optimized cut
    return this->CutGeneric(input, cutPolyData);
    }
  else
    {
    // even more optimized cut
    return this->CutAxisAligned(input, cutPolyData, axis);
    }
}

//------------------------------------------------------------------------
int vtkTriMeshPlaneCutter
::CutGeneric( vtkPolyData *input, vtkPolyData *cutPolyData )
{
  vtkSmartPointer< vtkPoints > cutPoints = vtkSmartPointer< vtkPoints >::New();
  vtkSmartPointer< vtkCellArray > cutLines
    = vtkSmartPointer< vtkCellArray >::New();
  cutPolyData->SetPoints(cutPoints);
  cutPolyData->SetLines(cutLines);


  vtkIdType cellId, ncellpts, *cellpts;
  vtkCellArray *inputPolys = input->GetPolys();
  vtkPoints *points = input->GetPoints();

  // The triangle points
  double trianglePts[3][3];

  // Intersection points and intersection point ids
  double intersectionPts[3][3];
  vtkIdType nCutLines = 0, lineEndPtIds[2];

  // loop over the triangles
  for (cellId=0, inputPolys->InitTraversal();
       inputPolys->GetNextCell(ncellpts, cellpts); cellId++)
    {

    // We run only on triangles. Let's assume it and skip the check below
    //if ( input->GetCellType(cellId) != VTK_TRIANGLE )
    //  {
    //  continue;
    //  }

    // Get the end points of the triangle
    points->GetPoint(cellpts[0], trianglePts[0]);
    points->GetPoint(cellpts[1], trianglePts[1]);
    points->GetPoint(cellpts[2], trianglePts[2]);

    // Intersect the triangle with the plane. At least 2 edges must intersect
    // if the triangle intersects. Hence a check here for at least two can be
    // done

    // Intersect the first edge with the plane.
    const bool e1Intersects =
      EdgePlaneIntersect(this->Origin, this->Normal,
        trianglePts[0], trianglePts[1], intersectionPts[0]);

    // Intersect the second edge with the plane
    const bool e2Intersects =
      EdgePlaneIntersect(this->Origin, this->Normal,
        trianglePts[1], trianglePts[2], intersectionPts[1]);

    // at least one of these must intersect, otherwise the triangle does
    // not intersect the plane
    if (e1Intersects == false && e2Intersects == false)
      {
      continue;
      }

    // At least one of the edges interesects. We can rest assured that there
    // will be two edges of this triangle that will intersect the plane
    lineEndPtIds[0] = nCutLines;
    lineEndPtIds[1] = lineEndPtIds[0]+1;

    if (e1Intersects == true && e2Intersects == true)
      {
      // We have both the end points if both intersect. Let's store them
      cutPoints->InsertNextPoint(intersectionPts[0]);
      cutPoints->InsertNextPoint(intersectionPts[1]);
      cutLines->InsertNextCell(2, lineEndPtIds);
      nCutLines += 2;
      }
    else if (e1Intersects == true && e2Intersects == false)
      {
      // only one has intersected. Let's get the other one.
      const bool e3Intersects =
        EdgePlaneIntersect(this->Origin, this->Normal,
          trianglePts[0], trianglePts[2], intersectionPts[2]);

      // This will always evaluate to true; nevertheless, let's check
      if (e3Intersects)
        {
        cutPoints->InsertNextPoint(intersectionPts[0]);
        cutPoints->InsertNextPoint(intersectionPts[2]);
        cutLines->InsertNextCell(2, lineEndPtIds);
        nCutLines += 2;
        }
      }
    else if (e1Intersects == false && e2Intersects == true)
      {
      // only one has intersected. Let's get the other one.
      const bool e3Intersects =
        EdgePlaneIntersect(this->Origin, this->Normal,
          trianglePts[0], trianglePts[2], intersectionPts[2]);

      // This will always evaluate to true; nevertheless, let's check
      if (e3Intersects)
        {
        cutPoints->InsertNextPoint(intersectionPts[1]);
        cutPoints->InsertNextPoint(intersectionPts[2]);
        cutLines->InsertNextCell(2, lineEndPtIds);
        nCutLines += 2;
        }
      }
    } // each cell

  return 1;
}

//------------------------------------------------------------------------
int vtkTriMeshPlaneCutter
::CutAxisAligned( vtkPolyData *input, vtkPolyData *cutPolyData, int axis )
{
  vtkSmartPointer< vtkPoints > cutPoints = vtkSmartPointer< vtkPoints >::New();
  vtkSmartPointer< vtkCellArray > cutLines
    = vtkSmartPointer< vtkCellArray >::New();
  cutPolyData->SetPoints(cutPoints);
  cutPolyData->SetLines(cutLines);


  vtkIdType cellId, ncellpts, *cellpts;
  vtkCellArray *inputPolys = input->GetPolys();
  vtkPoints *points = input->GetPoints();

  // The triangle points
  double trianglePts[3][3];

  // Intersection points and intersection point ids
  double intersectionPts[3][3];
  vtkIdType nCutLines = 0, lineEndPtIds[2];

  // loop over the triangles
  for (cellId=0, inputPolys->InitTraversal();
       inputPolys->GetNextCell(ncellpts, cellpts); cellId++)
    {

    // We run only on triangles. Let's assume it and skip the check below
    //if ( input->GetCellType(cellId) != VTK_TRIANGLE )
    //  {
    //  continue;
    //  }

    // Get the end points of the triangle
    points->GetPoint(cellpts[0], trianglePts[0]);
    points->GetPoint(cellpts[1], trianglePts[1]);
    points->GetPoint(cellpts[2], trianglePts[2]);

    // Intersect the triangle with the plane. At least 2 edges must intersect
    // if the triangle intersects. Hence a check here for at least two can be
    // done

    // Intersect the first edge with the plane.
    const bool e1Intersects =
      EdgePlaneIntersectAxisAligned(this->Origin, axis,
        trianglePts[0], trianglePts[1], intersectionPts[0]);

    // Intersect the second edge with the plane
    const bool e2Intersects =
      EdgePlaneIntersectAxisAligned(this->Origin, axis,
        trianglePts[1], trianglePts[2], intersectionPts[1]);

    // at least one of these must intersect, otherwise the triangle does
    // not intersect the plane
    if (e1Intersects == false && e2Intersects == false)
      {
      continue;
      }

    // At least one of the edges interesects. We can rest assured that there
    // will be two edges of this triangle that will intersect the plane
    lineEndPtIds[0] = nCutLines;
    lineEndPtIds[1] = lineEndPtIds[0]+1;

    if (e1Intersects == true && e2Intersects == true)
      {
      // We have both the end points if both intersect. Let's store them
      cutPoints->InsertNextPoint(intersectionPts[0]);
      cutPoints->InsertNextPoint(intersectionPts[1]);
      cutLines->InsertNextCell(2, lineEndPtIds);
      nCutLines += 2;
      }
    else if (e1Intersects == true && e2Intersects == false)
      {
      // only one has intersected. Let's get the other one.
      const bool e3Intersects =
        EdgePlaneIntersectAxisAligned(this->Origin, axis,
          trianglePts[0], trianglePts[2], intersectionPts[2]);

      // This will always evaluate to true; nevertheless, let's check
      if (e3Intersects)
        {
        cutPoints->InsertNextPoint(intersectionPts[0]);
        cutPoints->InsertNextPoint(intersectionPts[2]);
        cutLines->InsertNextCell(2, lineEndPtIds);
        nCutLines += 2;
        }
      }
    else if (e1Intersects == false && e2Intersects == true)
      {
      // only one has intersected. Let's get the other one.
      const bool e3Intersects =
        EdgePlaneIntersectAxisAligned(this->Origin, axis,
          trianglePts[0], trianglePts[2], intersectionPts[2]);

      // This will always evaluate to true; nevertheless, let's check
      if (e3Intersects)
        {
        cutPoints->InsertNextPoint(intersectionPts[1]);
        cutPoints->InsertNextPoint(intersectionPts[2]);
        cutLines->InsertNextCell(2, lineEndPtIds);
        nCutLines += 2;
        }
      }
    } // each cell

  return 1;
}

//-------------------------------------------------------------------------
void vtkTriMeshPlaneCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

