#include "vtkRTSSMPR.h"

#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkMarchingCubes.h"
#include "vtkCutter.h"
#include "vtkTriMeshPlaneCutter.h"
#include "vtkTimerLog.h"
#include "vtkMassProperties.h"
#include "vtkSmoothPolyDataFilter.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"
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
vtkStandardNewMacro(vtkRTSSMPR);

//------------------------------------------------------------------------
vtkRTSSMPR::vtkRTSSMPR()
{
  this->SubsampleFactor = 1;
  this->Surface = NULL;
  this->Volume = NULL;
  this->PixelToPatientTfm = vtkMatrix4x4::New();
  this->PatientToPixelTfm = vtkMatrix4x4::New();
  this->MassProperties = vtkMassProperties::New();

  this->Cutter =
#if RTSS_USE_OPTIMIZED_CUTTER
    vtkTriMeshPlaneCutter::New();
#else
    vtkCutter::New();
#endif

  this->PolygonsInPatientSpace = new PolygonContainerType();
  this->PolygonsInPixelSpace = new PolygonContainerType();

  this->SmoothSurface = 0;
  this->RelaxationFactor = 0.02;
  this->SmoothIterations = 10;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(0);

  // Initialize to zero and identity
  for (int i = 0; i < 3; i++)
    {
    this->Direction[0][i] = 0;
    this->Direction[1][i] = 0;
    this->InputResolution[i] = 0;
    this->Origin[i] = 0;
    this->InputNumberOfVoxels[i] = 0;
    }
  this->Direction[0][0] = 1;
  this->Direction[1][1] = 1;
}

//------------------------------------------------------------------------
vtkRTSSMPR::~vtkRTSSMPR()
{
  if (this->Surface)
    {
    this->Surface->Delete();
    }
  this->FreeVolume();
  this->PixelToPatientTfm->Delete();
  this->PatientToPixelTfm->Delete();
  this->Cutter->Delete();
  this->MassProperties->Delete();
  delete this->PolygonsInPatientSpace;
  delete this->PolygonsInPixelSpace;
}

//------------------------------------------------------------------------
void vtkRTSSMPR::SetImageGeometry(
    double origin[3],
    double spacing[3],
    int nVoxels[3],
    double directionX[3],
    double directionY[3] )
{
  for (int i = 0; i < 3; i++)
    {
    this->Direction[0][i] = directionX[i];
    this->Direction[1][i] = directionY[i];
    this->InputResolution[i] = spacing[i];
    this->Origin[i] = origin[i];
    this->InputNumberOfVoxels[i] = nVoxels[i];
    }

  this->UpdateSubsampledImageGeometry();

  // we need to regenerate the surface
  this->Modified();
}

//------------------------------------------------------------------------
void vtkRTSSMPR::UpdateSubsampledImageGeometry()
{
  // Compute the new number of voxels in-plane
  for (int i = 0; i < 2; i++)
    {
    this->NumberOfVoxels[i] = 
      ceil((double)this->InputNumberOfVoxels[i]/this->SubsampleFactor);
    this->Resolution[i] = this->InputResolution[i] * this->SubsampleFactor;
    }

  // Retain the sample factors out of plane. No change out of plane.
  this->NumberOfVoxels[2] = this->InputNumberOfVoxels[2];
  this->Resolution[2] = this->InputResolution[2];

  // Construct the transformation matrix that will take us
  // from pixel to patient coordinates.

  // The Z unit vector is Direction[2].
  vtkMath::Cross(this->Direction[0], this->Direction[1], this->Direction[2]);

  // Populate the 4x4 transform, comprising rotation, scale
  // and translation components.
  this->PixelToPatientTfm->Identity();
  for (int i = 0; i < 3; i++)
    {
    this->PixelToPatientTfm->SetElement(0,i,this->Resolution[0]*this->Direction[0][i]);
    this->PixelToPatientTfm->SetElement(1,i,this->Resolution[1]*this->Direction[1][i]);
    this->PixelToPatientTfm->SetElement(2,i,this->Resolution[2]*this->Direction[2][i]);
    this->PixelToPatientTfm->SetElement(i,3,this->Origin[i]);
    }

  // The PatientToPixelTfm is the inverse of the PixelToPatientTfm
  vtkMatrix4x4::Invert(this->PixelToPatientTfm, this->PatientToPixelTfm);

  this->CreateOrGetVolume()->SetExtent(0, this->NumberOfVoxels[0]-1,
                                       0, this->NumberOfVoxels[1]-1,
                                       0, this->NumberOfVoxels[2]-1);
}

//------------------------------------------------------------------------
void vtkRTSSMPR::AddContour(vtkPoints *pts)
{
  vtkSmartPointer< vtkPoints > contourPts =
    vtkSmartPointer< vtkPoints >::New();
  contourPts->DeepCopy(pts);
  this->PolygonsInPatientSpace->push_back(contourPts);

  // we need to regenerate the surface
  this->Modified();
}

//------------------------------------------------------------------------
void vtkRTSSMPR::RemoveAllContours()
{
  this->PolygonsInPatientSpace->clear();

  // we need to regenerate the surface
  this->Modified();
}

//------------------------------------------------------------------------
void vtkRTSSMPR::ClearVolume()
{
  unsigned char *ptr = (unsigned char *)(
      this->CreateOrGetVolume()->GetScalarPointer());

  const vtkIdType n = this->CreateOrGetVolume()->GetNumberOfPoints();
  for (vtkIdType i = 0; i < n; ++i, ++ptr)
    {
    *ptr = 0;
    }
}

//------------------------------------------------------------------------
void vtkRTSSMPR::PatientToPixel( double in[3], double out[3] )
{
  double inp[4] = { in[0], in[1], in[2], 1.0 }, outp[4];
  this->PatientToPixelTfm->MultiplyPoint(inp, outp);
  out[0] = outp[0];
  out[1] = outp[1];
  out[2] = outp[2];
}

//------------------------------------------------------------------------
int vtkRTSSMPR::GetContourSlice( vtkPoints *pts )
{
  // Get any point on the contour, since its co-planar.
  double in[3], out[3];
  pts->GetPoint(0,in);
  this->PatientToPixel(in, out);

  // in[2] should always be an integer, but let's be wary of any
  // precision issues and round it anyway.
  //std::cout << "  Slice: " << out[2] << " ";
  return static_cast< int >(out[2] + 0.5);
}

//------------------------------------------------------------------------
// In-place transform from pixel to patient coordinate system.
void vtkRTSSMPR::PixelToPatient( vtkPoints *pts )
{
  if (pts)
    {
    double inp[4] = { 0, 0, 0, 1.0 }, outp[4];
    for (unsigned int i = 0; i < pts->GetNumberOfPoints(); i++)
      {
      pts->GetPoint(i, inp);
      this->PixelToPatientTfm->MultiplyPoint(inp, outp);
      pts->SetPoint(i, outp);
      }
    }
}


//------------------------------------------------------------------------
typedef struct edge Edge;
struct edge
{
  int ymax;
  float x;
  float xincr;
  Edge* next;
};

//------------------------------------------------------------------------
void remove_old_edges (Edge** head, int y)
{
  Edge *p, *n;
  p = *head;
  while (p && p->ymax < y) p = p->next;
  *head = p;

  while (p)
    {
    n = p->next;
    while (n && n->ymax < y) n = n->next;
    p->next = n;
    p = n;
    }
}

//------------------------------------------------------------------------
void insert_ordered_by_x (Edge** head, Edge* c)
{
  Edge* p;
  if ((p = *head))
    {
    if (p->x > c->x)
      {
      c->next = p;
      *head = c;
      }
    else
      {
      while (p->next && p->next->x < c->x) p = p->next;
      c->next = p->next;
      p->next = c;
      }
    }
  else
    {
    *head = c;
    c->next = 0;
    }
}

//------------------------------------------------------------------------
void print_edges(Edge* p)
{
  while (p)
    {
    printf ("[%g %g %d] ", p->x, p->xincr, p->ymax);
    p = p->next;
    }
}


//------------------------------------------------------------------------
// Based on the scanline algorithm described in: "The Simplicity of Complex Polygons",
// Michael Abrash, Dr. Dobb's Journal June 1991, v16, n6, p139(6).
// It uses integer only arithmetic, Bresenham lines
// to keep track of edge lengths. This is the fastest known algorithm
// to rasterize contours.
//
// A detailed description can be found here:
//   http://www.nondot.org/sabre/Mirrored/GraphicsProgrammingBlackBook/gpbb40.pdf
// "Of Songs, Taxes and the simplicity of complex polygons"
// or refer to Section 3.6 of Foley and van Dam’s Computer Graphics, second edition
//
// There is even has an assembly version that ought to be faster..
//
void vtkRTSSMPR::RasterizeContour(
    vtkPoints *pts // polygon vertices in mm
)
{
  // number of points in the contour
  int num_vertices = pts->GetNumberOfPoints();

  if (num_vertices < 3)
    {
    // at least 3 points needed for a fill.
    return;
    }

  // the slice
  int z = (int)(pts->GetPoint(0)[2] + 0.5);

  // the discretized volume. Get the start pointer to the slice where
  // the contour lies
  unsigned char* imgp = (unsigned char*)(this->CreateOrGetVolume()->GetScalarPointer(
    this->ContourExtent[0],this->ContourExtent[2],z));
  //std::cout << "  Slice: " << z << std::endl;

  // the dimensions of the volume
  int dims[3];
  this->CreateOrGetVolume()->GetDimensions(dims);


  Edge** edge_table;
  Edge* edge_list;    /* Global edge list (GET) */
  Edge* ael;        /* Active edge list */
  double *x, *y;      /* vertices in pixel coordinates */

  // Check if last vertex == first vertex.
  // If so, discard it.
  double firstPt[3], lastPt[3];
  pts->GetPoint(0, firstPt);
  pts->GetPoint(num_vertices-1, lastPt);
  if (firstPt[0] == lastPt[0] &&
      firstPt[1] == lastPt[1] &&
      firstPt[2] == lastPt[2])
    {
    --num_vertices;
    }

  // X and Y contour points stored w.r.t the first pixel in the contour 
  // bounding box
  x = (double*) malloc (num_vertices * sizeof(double));
  y = (double*) malloc (num_vertices * sizeof(double));

  double p[3]; // contour pt
  for (int i = 0; i < num_vertices; i++)
    {
    pts->GetPoint(i, p);

    // Store in pixel coordinates as an offset w.r.t the contour pixel-bbox
    x[i] = p[0] - this->ContourExtent[0];
    y[i] = p[1] - this->ContourExtent[2];
    }

  /* Make edge table */
  edge_table = (Edge**) malloc (dims[1] * sizeof(Edge*));
  edge_list = (Edge*) malloc (num_vertices * sizeof(Edge));
  memset (edge_table, 0, dims[1] * sizeof(Edge*));

  // loop over all the vertices
  for (size_t i = 0; i < num_vertices; i++)
    {
    int ymin, ymax;
    size_t a = i, b = (i==num_vertices-1 ? 0 : i+1);

    /* Reorder segment so that y[a] > y[b] */
    if (y[a] == y[b]) continue;
    if (y[a] < y[b]) a = b, b = i;

    /* Reject segments too high or too low */
    ymin = (int) ceil(y[b]);
    if (ymin > dims[1]-1) continue;
    ymax = (int) floor(y[a]);
    if (ymax < 0) continue;

    /* If upper y lies on scan line, don't count it as
     * an intersection */
    if (y[a] == ymax) ymax --;

    /* Reject segments that don't intersect a scan line */
    if (ymax < ymin) continue;

    /* Clip segments against image boundary */
    if (ymin < 0) ymin = 0;
    if (ymax > dims[1]-1) ymax = dims[1]-1;

    /* Shorten the segment & fill in edge data */
    edge_list[i].ymax = ymax;
    edge_list[i].xincr = (x[a] - x[b]) / (y[a] - y[b]);
    edge_list[i].x = x[b] + (ymin - y[b]) * edge_list[i].xincr;
    edge_list[i].next = 0;

    /* Insert into edge_table */
#if DEBUGGNG
  printf ("[y:%g %g, x:%g %g] -> [y:%d %d, x:%g (%g)]\n",
      y[b], y[a], x[b], x[a],
      ymin, ymax, edge_list[i].x, edge_list[i].xincr);
#endif
    insert_ordered_by_x (&edge_table[ymin], &edge_list[i]);
    }

    /* Debug edge table */
#if DEBUGGNG
  printf ("-------------------------------------------\n");
  for (int i = 0; i < dims[1]; i++)
    {
    if (edge_table[i])
      {
      printf ("%d: ", i);
      print_edges (edge_table[i]);
      printf ("\n");
      }
    }
#endif

  /* Loop through scanline, rendering each */
  ael = 0;
  for (int i = 0; i < dims[1]; i++)
    {
    int x, num_crossings;
    Edge *n, *c;

    /* Remove old edges from AEL */
    remove_old_edges (&ael, i);

    /* Add new edges to AEL */
    c = edge_table[i];
    while (c)
      {
      n = c->next;
      insert_ordered_by_x (&ael, c);
      c = n;
      }

    /* Count scan intersections & rasterize */
    num_crossings = 0;
    x = 0;
    c = ael;
#if DEBUGGNG
    printf ("%d ", i);
    print_edges (ael);
#endif
    while (x < dims[0])
      {
      int next_x;
      while (1)
        {
        if (!c)
          {
          next_x = dims[0];
          break;
          }
        else if (x >= c->x)
          {
          c = c->next;
          num_crossings ++;
          continue;
          }
        else
          {
          next_x = (int) floor (c->x) + 1;
          if (next_x > dims[0]) next_x = dims[0];
          break;
          }
        }

      num_crossings = num_crossings % 2;
#if DEBUGGNG
      printf ("(%d %c %d)", x, num_crossings?'+':'-', next_x-1);
#endif
      while (x < next_x)
        {
        // Handle concentric holes and multiple contours. ie the
        // rasterizer first checks if we are inside, and if so XORs
        // the result with the existing discretized map
        if (num_crossings)
          {
          *imgp ^= (unsigned char)num_crossings;
          }
        ++imgp;
        x++;
        }
      }
#if DEBUGGNG
    printf ("\n");
    getchar();
#endif

  /* Update x values on AEL */
    c = ael;
    while (c)
      {
      c->x += c->xincr;
      c = c->next;
      }

  /* Resort AEL - this could be done more efficiently */
    c = ael;
    while (c)
      {
      if (c->next && c->x > c->next->x)
        {
        Edge* tmp = c->next;
        c->next = c->next->next;
        insert_ordered_by_x (&ael, tmp);
        c = tmp;
        }
      else
        {
        c = c->next;
        }
      }
    }

  /* Free things up */
  free (x);
  free (y);
  free (edge_table);
  free (edge_list);
}

//------------------------------------------------------------------------
// Transform the contours to pixel space. Contour is assumed to be planar
// and have a constant Z when transformed to pixel space.
vtkSmartPointer< vtkPoints > vtkRTSSMPR::TransformContourToPixel( vtkPoints *pts )
{
  if (pts->GetNumberOfPoints() < 1)
    {
    return NULL;
    }
  
  // the transform matrix that takes us from the patient to pixel space.
  double *tfm = *(this->PatientToPixelTfm->Element);

  const int num_vertices = pts->GetNumberOfPoints();

  vtkSmartPointer< vtkPoints > ptsTransformed = vtkSmartPointer< vtkPoints >::New();
  ptsTransformed->SetNumberOfPoints( num_vertices );

  // z[i] = .. Since its a coplanar contour, all the z's are
  //          going to be the same. Hence we directly offset to the
  //          slice pointer when populating the discrete volume.
  const int z = this->GetContourSlice(pts);

  // Update the running bounds
  if (this->ContourExtent[4] > (int)z) this->ContourExtent[4] = (int)z;
  if (this->ContourExtent[5] < (int)z) this->ContourExtent[5] = (int)z;

  // Convert from mm to pixel coords
  // transform the contour points from patient to pixel space.

  double p[3]; // contour pt
  double pTx, pTy; // transformed contour point
  for (int i = 0; i < num_vertices; i++)
    {
    pts->GetPoint(i, p);
    
    // Transform the X and Y coords
    pTx = tfm[0]*p[0] + tfm[1]*p[1] + tfm[2]*p[2] + tfm[3];
    pTy = tfm[4]*p[0] + tfm[5]*p[1] + tfm[6]*p[2] + tfm[7];

    // Update the running bounds
    if (this->ContourExtent[0] > (int)pTx) this->ContourExtent[0] = (int)pTx;
    if (this->ContourExtent[1] < (int)pTx) this->ContourExtent[1] = (int)pTx;
    if (this->ContourExtent[2] > (int)pTy) this->ContourExtent[2] = (int)pTy;
    if (this->ContourExtent[3] < (int)pTy) this->ContourExtent[3] = (int)pTy;

    ptsTransformed->SetPoint(i, pTx, pTy, z);
    }

  return ptsTransformed;
}

//------------------------------------------------------------------------
void vtkRTSSMPR::TransformContoursToPixel()
{
  this->ContourExtent[0] = VTK_INT_MAX;
  this->ContourExtent[1] = VTK_INT_MIN;
  this->ContourExtent[2] = VTK_INT_MAX;
  this->ContourExtent[3] = VTK_INT_MIN;
  this->ContourExtent[4] = VTK_INT_MAX;
  this->ContourExtent[5] = VTK_INT_MIN;

  this->PolygonsInPixelSpace->clear();
  this->PolygonsInPixelSpace->resize(this->PolygonsInPatientSpace->size());

  // loop through all polygons, discretizing them into a binary volume
  PolygonContainerType::iterator oit = this->PolygonsInPixelSpace->begin();
  for (PolygonContainerType::iterator pit = this->PolygonsInPatientSpace->begin();
      pit != this->PolygonsInPatientSpace->end();
      ++pit, ++oit)
    {
    // rasterize this contour into this->Volume.
    *oit = this->TransformContourToPixel( (*pit).GetPointer() );
    }
}

//------------------------------------------------------------------------
// Dilate bounds by a pixel along each dimension, taking care not to get
// larger than the volume extents
void vtkRTSSMPR::DilateContourExtent()
{
  // Dilate the contour extent taking care not to get larger than the
  // DICOM volume geometry. This creates an axis aligned (aligned with
  // the DICOM direction cosines) tight bounding box that's as large as the
  // contour and no more. Its enlarged by a pixel to allow marching cubes
  // to extract the isosurface correctly.

  for (int i = 0; i < 3; i++)
    {
    this->ContourExtent[2*i] = this->ContourExtent[2*i]-1;
    if (this->ContourExtent[2*i] < 0)
      {
      this->ContourExtent[2*i] = 0;
      }
    this->ContourExtent[2*i+1] = this->ContourExtent[2*i+1]+1;
    if (this->ContourExtent[2*i+1] > (this->NumberOfVoxels[i]-1))
      {
      this->ContourExtent[2*i+1] = (this->NumberOfVoxels[i]-1);
      }
    }
}

//------------------------------------------------------------------------
void vtkRTSSMPR::AllocateVolume()
{
  // Dilate bounds by a pixel along each dimension, taking care not to get
  // larger than the volume extents
  this->DilateContourExtent();

  // Set the extents to the subregion.
  this->CreateOrGetVolume()->SetExtent(this->ContourExtent);

  // Allocate the volume.
# if (VTK_MAJOR_VERSION >= 6)
  this->CreateOrGetVolume()->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
#else
  this->CreateOrGetVolume()->SetScalarTypeToUnsignedChar();
  this->CreateOrGetVolume()->AllocateScalars();
#endif
}

//------------------------------------------------------------------------
void vtkRTSSMPR::Discretize()
{
  // Transform the contours to pixel space
  this->TransformContoursToPixel();

  // Allocate the volume
  this->AllocateVolume();

  // Clear the volume buffer.
  this->ClearVolume();

  // loop through all polygons, discretizing them into a binary volume
  for (PolygonContainerType::iterator pit = this->PolygonsInPixelSpace->begin();
      pit != this->PolygonsInPixelSpace->end();
      ++pit)
    {
    // rasterize this contour into this->Volume.
    this->RasterizeContour( (*pit).GetPointer() );
    }

  // update the volume's timestamp.
  this->CreateOrGetVolume()->Modified();
}

//------------------------------------------------------------------------
void vtkRTSSMPR::ExtractSurface()
{
  TIMER_START(surfaceComputeTimer);

  vtkSmartPointer< vtkMarchingCubes > mc = vtkSmartPointer< vtkMarchingCubes >::New();
#if (VTK_MAJOR_VERSION >= 6)
  mc->SetInputData(this->CreateOrGetVolume());
#else
  mc->SetInput(this->CreateOrGetVolume());
#endif

  // Isovalue at the middle. The expectation of the error due to
  // discretization will therefore be 0, given an infinite number
  // of samples and given that the contour edge and the volume
  // are IID's (Identical and Independent Distributed) random variables.
  // Thus the contour distrubution w.r.t the voxel is uniformly
  // distributed and therefore although maximum error in discretization
  // is half a voxel, the expectation of this error is 0.
  mc->SetValue(0, 0.5);

  mc->Update();

  // smooth if user wishes to
  vtkSmartPointer< vtkSmoothPolyDataFilter > smoother =
    vtkSmartPointer< vtkSmoothPolyDataFilter >::New();
  smoother->SetInputConnection(mc->GetOutputPort());
  smoother->SetRelaxationFactor(this->RelaxationFactor);
  smoother->SetNumberOfIterations(this->SmoothIterations);
  smoother->FeatureEdgeSmoothingOff();
  smoother->BoundarySmoothingOn();
  smoother->GenerateErrorScalarsOff();
  smoother->GenerateErrorVectorsOff();

  if (this->SmoothSurface)
    {
    smoother->Update();
    this->Surface->ShallowCopy( smoother->GetOutput() );
    }
  else
    {
    // disconnect surface from the pipeline
    this->Surface->ShallowCopy( mc->GetOutput() );
    }

  // Transform surfaceback to patient coords
  this->PixelToPatient(this->Surface->GetPoints());  

  this->Surface->Modified();

  TIMER_STOP(surfaceComputeTimer, "Surface has: " <<
    (this->Surface->GetPoints() ? this->Surface->GetPoints()->GetNumberOfPoints() : 0)
    << " points and " << this->Surface->GetNumberOfPolys() << " faces.")
}

//------------------------------------------------------------------------
void vtkRTSSMPR::WriteSurface()
{
#ifdef DEBUGGING
  // Construct a filename with the number of verts and faces, so that multiple
  // structures, whose names are not known, don't overwrite each other into
  // the same file.
  std::ostringstream filename;
  filename << "Surface_" << this->Surface->GetPoints()->GetNumberOfPoints()
           << "pts_" << this->Surface->GetNumberOfPolys() << "Tris.vtk"
           << std::ends;

  vtkSmartPointer< vtkPolyDataWriter > writer =
    vtkSmartPointer< vtkPolyDataWriter >::New();
  writer->SetFileName(filename.str().c_str());
#  if (VTK_MAJOR_VERSION >= 6)
  writer->SetInputData(this->Surface);
#  else
  writer->SetInput(this->Surface);
#  endif
  writer->Write();
#endif
}

//------------------------------------------------------------------------
vtkPolyData *vtkRTSSMPR::GetSurface()
{
  this->Update();
  return this->Surface;
}

//------------------------------------------------------------------------
vtkSmartPointer< vtkPolyData > vtkRTSSMPR::GetCut( vtkPlane *plane )
{
  TIMER_START( cutTimer );

  this->Update();
#  if (VTK_MAJOR_VERSION >= 6)
  this->Cutter->SetInputData(this->Surface);
#  else
  this->Cutter->SetInput(this->Surface);
#  endif
  this->Cutter->SetCutFunction( plane );
  this->Cutter->Update();
  vtkSmartPointer< vtkPolyData > pd = vtkSmartPointer< vtkPolyData >::New();
  pd->DeepCopy(this->Cutter->GetOutput());
  TIMER_STOP( cutTimer, "Cut produced " << pd->GetPoints()->GetNumberOfPoints()
    << " points and " << pd->GetNumberOfLines() << " lines: ");
  return pd;
}

//------------------------------------------------------------------------
double vtkRTSSMPR::GetVolume()
{
  this->Update();
# if (VTK_MAJOR_VERSION >= 6)
  this->MassProperties->SetInputData(this->Surface);
# else
  this->MassProperties->SetInput(this->Surface);
# endif

  TIMER_START(volumeComputeTimer);

  this->MassProperties->Update();

  TIMER_STOP(volumeComputeTimer,
    " Volume = " << this->MassProperties->GetVolume() << " mm3 ");

  return this->MassProperties->GetVolume();
}

//------------------------------------------------------------------------
int vtkRTSSMPR::RequestData(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *)
{
  // Clear the surface.
  if (!this->Surface)
    {
    this->Surface = vtkPolyData::New();
    }
  this->Surface->Initialize();

  int extent[6];
  this->CreateOrGetVolume()->GetExtent(extent);
  if (!(extent[1] > extent[0] &&
        extent[3] > extent[2] &&
        extent[5] > extent[4]))
    {
    // invalid image geometry. We need a 3D volume. There is little sense in
    // providing reslize support for a single slice volume.
    return 1;
    }

  if (this->PolygonsInPatientSpace->size() == 0)
    {
    // No contours to reslize.
    return 1;
    }

  // Discretize
  this->Discretize();

  // Extract surface
  this->ExtractSurface();
 
  // Clear the volume
  this->FreeVolume();

  return 1;
}

//-------------------------------------------------------------------------
vtkImageData *vtkRTSSMPR::CreateOrGetVolume()
{
  if (!this->Volume)
    {
    this->Volume = vtkImageData::New();
    }
  return this->Volume;
}

//-------------------------------------------------------------------------
void vtkRTSSMPR::FreeVolume()
{
  if (this->Volume)
    {
    this->Volume->Delete();
    this->Volume = NULL;
    }
}

void vtkRTSSMPR::SetSubsampleFactor(double f)
{
  if (this->SubsampleFactor != f)
    {
    this->SubsampleFactor = f;
    this->UpdateSubsampledImageGeometry();
    this->Modified();
    }
} 

//-------------------------------------------------------------------------
void vtkRTSSMPR::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

