#ifndef __DCMGeometry_h
#define __DCMGeometry_h

//---------------------------------------------------------------------------
// declspec(dll import/export) stuff, when building via shared libraries
#ifdef WIN32
#ifndef BUILD_SHARED_LIBS // static lib
#  define DCMGEOMETRY_EXPORT
#else
#  ifdef DCMGEOMETRY_EXPORT // dynamic lib - must export/import symbols
#    define DCMGEOMETRY_EXPORT __declspec(dllexport)
#  else
#    define DCMGEOMETRY_EXPORT __declspec(dllimport)
#  endif
#endif
#else
// Compiler that is not MS Visual C++.
// Ensure that the export symbol is defined (and blank)
#define DCMGEOMETRY_EXPORT
#endif

#include "vtkObject.h"
#include "enums.h"

class DCMGEOMETRY_EXPORT DCMGeometry : public vtkObject
{
public:
	static DCMGeometry* New();
	vtkTypeMacro(DCMGeometry, vtkObject);
	void PrintSelf(ostream& os, vtkIndent indent);
	// Description:
	// Set the image geometry on which the DICOMS are defined.
	//The image geometry serves two purposes.This is used as a way of
	// specifying the frame of reference on which the contours are defined.
	// The origin, direction, spacing and nVoxels together define the cuboid
	// comprising the CT/MR/US/PET volume. This cuboid has its corners (patient
	// coordinates) at:
	// - origin
	// - origin + directionX * nVoxelsX * spacingX
	// - origin + directionY * nVoxelsY * spacingY
	// - origin + directionZ * nVoxelsZ * spacingZ
	void SetImageGeometry(double origin[3],
		double spacing[3],
		int nVoxels[3],
		double directionX[3],
		double directionY[3]);

	double ComputePosition(int sliceIdx, RTViewer::Axis axis);
	int ComputeIndex(double pos, RTViewer::Axis axis);
	int ComputeIndex2(double pos, RTViewer::Axis axis);
	bool CheckBounds(double pos, RTViewer::Axis axis);

	DCMGeometry* GetPointer();

	void GetOrigin(double* origin);
	void GetLastImagePosition(double* pos);
	void GetDimensions(int* dim);
	void GetOrientation(double* x, double* y, double* z);
	void GetSpacing(double* spacing);
	void GetBounds(double* bounds);
	void GetExtent(int* extent);

	

protected:
	DCMGeometry();
	~DCMGeometry();

	// Input image geometry
	int InputNumberOfVoxels[3];
	double InputResolution[3];
	double Direction[3][3];
	double Origin[3];
	int Extent[6];
	double Bound[6];

};
#endif
//class DICOMGeometry
//{
//protected:
//	DICOMGeometry(double);
//	~DICOMGeometry();
//
//private:
//	double m_origin[3];
//	int m_dimensions[3];
//	double m_bounds[6];
//	int m_extent[6];
//	double m_spacing[3];
//	double m_xOrient[3];
//	double m_yOrient[3];
//	double m_zOrient[3];
//
//	friend class MPR;
//};