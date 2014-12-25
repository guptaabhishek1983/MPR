#ifndef __vtkTriMeshPlaneCutter_h
#define __vtkTriMeshPlaneCutter_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include <vector>

//---------------------------------------------------------------------------
// declspec(dll import/export) stuff, when building via shared libraries
#ifdef WIN32
  #ifndef BUILD_SHARED_LIBS // static lib
  #  define RTSSMPR_EXPORT
  #else
  #  ifdef RTSSMPR_EXPORTS // dynamic lib - must export/import symbols
  #    define RTSSMPR_EXPORT __declspec(dllexport)
  #  else
  #    define RTSSMPR_EXPORT __declspec(dllimport)
  #  endif
  #endif
#else
  // Compiler that is not MS Visual C++.
  // Ensure that the export symbol is defined (and blank)
  #define RTSSMPR_EXPORT
#endif

//---------------------------------------------------------------------------
// Forward declarations
class vtkPlane;

//---------------------------------------------------------------------------
// Fast intersection of a triangle mesh and a plane. The input is the triangle
// mesh to be cut. Set a point on the plane and its normal via SetOrigin and
// SetNormal. Then update the filter and call GetOutput to get the cut
// polydata.
class RTSSMPR_EXPORT vtkTriMeshPlaneCutter : public vtkPolyDataAlgorithm
{
public:

  // Description:
  // Standard methods for printing and determining type information.
  static vtkTriMeshPlaneCutter *New();
  vtkTypeMacro(vtkTriMeshPlaneCutter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the plane Normal
  vtkSetVector3Macro( Normal, double );
  vtkGetVector3Macro( Normal, double );

  // Description:
  // Set any point on the plane
  vtkSetVector3Macro( Origin, double );
  vtkGetVector3Macro( Origin, double );

  // Description:
  // Alternative way to set the cut function by specifying the plane parameters
  void SetCutFunction( vtkPlane * );

protected:
  vtkTriMeshPlaneCutter();
  ~vtkTriMeshPlaneCutter();

  virtual int RequestData( vtkInformation *,
                           vtkInformationVector **,
                           vtkInformationVector *);

  // Optimized for a generic plane, trimesh cut, by triangle, plane
  // fast intersections
  int CutGeneric( vtkPolyData *input, vtkPolyData *cutPolyData );

  // Optimized even more for a generic plane, trimesh cut, for axis
  // aligned planes. 'axis' can be 0,1,2 for X,Y,Z axis normals
  int CutAxisAligned( vtkPolyData *input, vtkPolyData *cutPolyData, int axis );

  double Origin[3];
  double Normal[3];

private:
  vtkTriMeshPlaneCutter(const vtkTriMeshPlaneCutter&);  // Not implemented.
  void operator=(const vtkTriMeshPlaneCutter&);  // Not implemented.
};

#endif
