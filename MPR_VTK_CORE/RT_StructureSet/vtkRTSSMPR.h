#ifndef __vtkRTSSMPR_h
#define __vtkRTSSMPR_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkCutter.h"
#include "vtkTriMeshPlaneCutter.h"
#include <vector>

//---------------------------------------------------------------------------
// Use the optimized cutter ? A value of 1 uses the optimized one
// (vtkTriMeshPlaneCutter), a value of 0 uses vtkCutter
#define RTSS_USE_OPTIMIZED_CUTTER 1

//---------------------------------------------------------------------------
// Forward declarations
class vtkPlane;
class vtkImageData;
class vtkMassProperties;
class vtkTriMeshPlaneCutter;
class vtkMatrix4x4;

//---------------------------------------------------------------------------
class RTSSMPR_EXPORT vtkRTSSMPR : public vtkPolyDataAlgorithm
{
public:

  // Description:
  // Standard methods for printing and determining type information.
  static vtkRTSSMPR *New();
  vtkTypeMacro(vtkRTSSMPR,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Each ROI sequence in a structure set comprises one or more contour
  // polygons. This method may be used to add a contour polygon. It is
  // assumed that the polygons are of type CLOSED_PLANAR (See tag (3006,0042)
  // "Contour Geometric Type" ftp://medical.nema.org/medical/dicom/final/sup11_ft.pdf
  // Section C.8.X.6).
  // The points passed in are expected to be an ordered list, as obtained from
  // DICOM
  // The points are explicitly copied, ie memory ownership is not transferred.
  // The points are in patient coordinates, (as obtained directly from DICOM)
  void AddContour( vtkPoints *pts );

  // Description:
  // Remove all RTRoi polygons.
  void RemoveAllContours();

  // Description:
  // Set the image geometry on which the structures are defined. To do so,
  // set the origin and spacing of the volume and its X and Y direction
  // cosine unit vectors. Note that the origin is the corner of the bounding
  // box. The image geometry serves two purposes. This is used as a way of
  // specifying the frame of reference on which the contours are defined.
  // The origin, direction, spacing and nVoxels together define the cuboid
  // comprising the CT/MR/US/PET volume. This cuboid has its corners (patient
  // coordinates) at:
  // - origin
  // - origin + directionX * nVoxelsX * spacingX
  // - origin + directionY * nVoxelsY * spacingY
  // - origin + directionZ * nVoxelsZ * spacingZ
  // It is assumed that the structures are contained entirely within this
  // cuboid. If they are not, only portions of the structures within this
  // cuboid will be considered. (ie the structures will be clipped by the
  // cuboid)
  void SetImageGeometry( double origin[3],
                         double spacing[3],
                         int nVoxels[3],
                         double directionX[3],
                         double directionY[3] );

  // Description:
  // Get the surface reconstructed from the contour set. This is a manifold
  // triangle mesh.
  vtkPolyData * GetSurface();

  // Description:
  // Get a polydata consisting of the intersection of the plane and this
  // structure. The plane may be defined by a point (lying on the plane) and a
  // normal.
  vtkSmartPointer< vtkPolyData > GetCut( vtkPlane *plane );

  // Description:
  // Get the volume contained within the structure.
  double GetVolume();

  // Description:
  // Set the subsample factor. A value of 1 amounts to no subsampling. A value
  // greater than 1 sacrifices accuracy at the cost of speed. The improvement in
  // speed is present both in the initial surface build time and in the time
  // taken for each cut. A value of less than 1 increases accuracy, (at the cost
  // of reduced speed) however is in practice meaningless.
  //   In theory, the improvement in speed is O(n^2) where n is the subsample
  // factor. Reasonable values for SubsampleFactor range from 1 to 2. The default
  // value is 1.5.
  //   Subsampling is never applied in the Z direction to avoid rasterization
  // issues of the contour. Its only applied in-plane.
  virtual void SetSubsampleFactor( double );
  vtkGetMacro( SubsampleFactor, double );

  // Description:
  // Smooth the surface. This will cause the initial surface generation from
  // the contour set to be slower. ie the first call to filter->Update() will
  // be slower. Cutting the surface will still take the same amount of time.
  // OFF by default.
  vtkSetMacro( SmoothSurface, int );
  vtkGetMacro( SmoothSurface, int );
  vtkBooleanMacro( SmoothSurface, int );

  // Description:
  // Set the smoothing number of iterations. Defaults to 10. More iterations
  // consume more time
  vtkSetMacro( SmoothIterations, int );
  vtkGetMacro( SmoothIterations, int );

  // Description:
  // Set the smoothing mesh relaxation factor. Reasonable values range from
  // [0.01 to 0.05]. Larger relaxation factors result in oversmoothing and
  // can cause the surface to turn non-manifold.
  vtkSetMacro( RelaxationFactor, int );
  vtkGetMacro( RelaxationFactor, int );

protected:
  vtkRTSSMPR();
  ~vtkRTSSMPR();

  void ExtractSurface();
  void Discretize();
  void RasterizeContour(vtkPoints *contourPts);
  void AllocateVolume();
  void ClearVolume();
  int GetContourSlice( vtkPoints *pts );
  void WriteSurface();
  void PatientToPixel( double in[3], double out[3] );
  void PixelToPatient( vtkPoints *pts );
  vtkSmartPointer< vtkPoints > TransformContourToPixel( vtkPoints *pts );
  void TransformContoursToPixel();
  void DilateContourExtent();
  void UpdateSubsampledImageGeometry();
  
  virtual int RequestData( vtkInformation *,
                           vtkInformationVector **,
                           vtkInformationVector *);

  vtkImageData * CreateOrGetVolume();
  void FreeVolume();
  vtkImageData * Volume;

  vtkPolyData * Surface;
  vtkMassProperties *MassProperties;

#if RTSS_USE_OPTIMIZED_CUTTER
  vtkTriMeshPlaneCutter *Cutter;
#else
  vtkCutter *Cutter;
#endif

  // The matrix takes a point from a pixel index to a point in the patient
  // coordinate system.
  vtkMatrix4x4 * PixelToPatientTfm;

  // The inverse of the above.
  vtkMatrix4x4 * PatientToPixelTfm;

  // The planar polygons supplied by the user
  typedef std::vector< vtkSmartPointer< vtkPoints > > PolygonContainerType;
  PolygonContainerType *PolygonsInPatientSpace;
  PolygonContainerType *PolygonsInPixelSpace;

  // Smooth ?
  int SmoothSurface;
  int SmoothIterations;
  double RelaxationFactor;

  // Input image geometry
  int InputNumberOfVoxels[3];
  double InputResolution[3];
  double Direction[3][3];
  double Origin[3];

  // Subsampled image geometry, which may be the same as the input geometry
  int NumberOfVoxels[3];
  double Resolution[3];

  // A value more than 1, can speed up the computation of the surface, at the
  // cost of accuracy of the surface
  double SubsampleFactor;

  // The bounds in pixel space taken up the contour. This is internally computed
  // as a by-product of the transformation of the contour from patielt to pixel
  // space
  int ContourExtent[6];

private:
  vtkRTSSMPR(const vtkRTSSMPR&);  // Not implemented.
  void operator=(const vtkRTSSMPR&);  // Not implemented.
};

#endif

