#include "DCMGeometry.h"
#include "rad_logger.h"
#include "rad_template_helper.h"
#include "rad_util.h"

#define __FILENAME__ "DCMGeometry.cpp"
#undef  __MODULENAME__
#define __MODULENAME__ "DCMGeometry"


#include "vtkObjectFactory.h"
#include "vtkMath.h"

#define Xmin 0
#define Xmax 1
#define Ymin 2
#define Ymax 3
#define Zmin 4
#define Zmax 5
//------------------------------------------------------------------------
vtkStandardNewMacro(DCMGeometry);

//------------------------------------------------------------------------
DCMGeometry::DCMGeometry()
{
	for (int i = 0; i < 3; i++)
	{
		this->Direction[0][i] = 0;
		this->Direction[1][i] = 0;
		this->InputResolution[i] = 0;
		this->Origin[i] = 0;
		this->InputNumberOfVoxels[i] = 0;
	}

	for (int i = 0; i < 6; i++)
	{
		Extent[i] = 0;
		Bound[i] = 0;
	}
}

DCMGeometry::~DCMGeometry()
{

}
//------------------------------------------------------------------------
void DCMGeometry::SetImageGeometry(double origin[3],
	double spacing[3],
	int nVoxels[3],
	double directionX[3],
	double directionY[3])

{
 	for (int i = 0; i < 3; i++)
	{
		this->Direction[0][i] = directionX[i];
		this->Direction[1][i] = directionY[i];
		this->InputResolution[i] = spacing[i];
		this->Origin[i] = origin[i];
		this->InputNumberOfVoxels[i] = nVoxels[i];
	}

	// The Z unit vector is Direction[2].
	vtkMath::Cross(this->Direction[0], this->Direction[1], this->Direction[2]);

	// set extent
	int j = 0;
	for (int i = 0; i < 3; i++)
	{
		Extent[j++] = 0;
		Extent[j++] = InputNumberOfVoxels[i] - 1;
	}
	
	// compute bounds. 
	//Bounds are stored as (Xmin, Xmax, Ymin, Ymax, Zmin, Zmax)
	j = 0;
	for (int i = 0; i< 3; i++)
	{
		Bound[j++] = this->Origin[i];
		Bound[j++] = this->Origin[i] + this->Direction[i][i] * Extent[j] * this->InputResolution[i];
	}

	RAD_LOG_CRITICAL("Bounds:" << Bound[0] << ":" << Bound[1] << ":" << Bound[2] << ":" << Bound[3] << ":" << Bound[4] << ":" << Bound[5]);
}

//------------------------------------------------------------------------
double DCMGeometry::ComputePosition(int sliceIdx, RTViewer::Axis axis)
{
	double pos = NULL;
	switch (axis)
	{
		case RTViewer::AxialAxis:
			pos = (this->Origin[2] + sliceIdx * this->Direction[2][2] * this->InputResolution[2]);
			break;

		case RTViewer::SagittalAxis:
			pos = this->Origin[0] + sliceIdx * this->Direction[0][0] * this->InputResolution[0];
			break;

		case RTViewer::CoronalAxis:
			pos = this->Origin[1] + sliceIdx * this->Direction[1][1] * this->InputResolution[1];
			break;
		default:
			break;
	}
	return pos;
}

int DCMGeometry::ComputeIndex(double pos, RTViewer::Axis axis)
{
	int idx = -1; // assuming index starts from 0.
	switch (axis)
	{
		case RTViewer::AxialAxis:
			if (CheckBounds(pos, axis))
				idx = (pos-this->Origin[2]) / (this->Direction[2][2] * this->InputResolution[2]);
			break;

		case RTViewer::SagittalAxis:
			if (CheckBounds(pos, axis))
				idx = (pos - this->Origin[0]) / (this->Direction[0][0] * this->InputResolution[0]);
			break;

		case RTViewer::CoronalAxis:
			if (CheckBounds(pos, axis))
				idx = (pos - this->Origin[1]) / (this->Direction[1][1] * this->InputResolution[1]);
			break;
		default:
			break;
	}
	return abs(idx);
}

int DCMGeometry::ComputeIndex2(double pos, RTViewer::Axis axis)
{
	int idx = -1; // assuming index starts from 0.
	switch (axis)
	{
		case RTViewer::AxialAxis:
			if (CheckBounds(pos, axis))
			{
				idx = (pos - this->Origin[2]) / (this->Direction[2][2] * this->InputResolution[2]);
				idx = this->Extent[Zmax] - idx;
			}
			break;

		case RTViewer::SagittalAxis:
			if (CheckBounds(pos, axis))
			{
				idx = (pos - this->Origin[0]) / (this->Direction[0][0] * this->InputResolution[0]);
				idx = this->Extent[Xmax] - idx;
			}
			break;

		case RTViewer::CoronalAxis:
			if (CheckBounds(pos, axis))
			{
				idx = (pos - this->Origin[1]) / (this->Direction[1][1] * this->InputResolution[1]);
				idx = this->Extent[Ymax] - idx;
			}
			break;
		default:
			break;
	}
	return abs(idx);
}

bool DCMGeometry::CheckBounds(double pos, RTViewer::Axis axis)
{
	bool inBounds = false;
	switch (axis)
	{
		case RTViewer::AxialAxis:
		{
			if (pos > Bound[Zmin] && pos < Bound[Zmax])
				inBounds = true;

			if (!inBounds && (pos > Bound[Zmax] && pos < Bound[Zmin]))
				inBounds = true;
		}
			break;

		case RTViewer::SagittalAxis:
		{
			if (pos > Bound[Xmin] && pos < Bound[Xmax])
			inBounds = true;
			if (!inBounds && (pos > Bound[Xmax] && pos < Bound[Xmin]))
				inBounds = true;
		}
			break;

		case RTViewer::CoronalAxis:
		{
			if (pos > Bound[Ymin] && pos < Bound[Ymax])
				inBounds = true;
			if (!inBounds && (pos > Bound[Ymax] && pos < Bound[Ymin]))
				inBounds = true;
		}
			break;
		default:
			break;
	}
	return inBounds;
}
//------------------------------------------------------------------------
DCMGeometry* DCMGeometry::GetPointer()
{
	return this;
}
//------------------------------------------------------------------------

void DCMGeometry::GetOrigin(double* origin){
	for (int i = 0; i < 3; i++)
	{
		origin[i] = Origin[i];
	}
}
void DCMGeometry::GetLastImagePosition(double* pos){
	for (int i = 0; i < 3; i++)
	{
		pos[i] = Origin[i];
	}
	pos[2] = Bound[Zmax];

}
void DCMGeometry::GetDimensions(int* dim){
	for (int i = 0; i < 3; i++)
	{
		dim[i] = InputNumberOfVoxels[i];
	}
}
void DCMGeometry::GetOrientation(double* x, double* y, double* z){
	for (int i = 0; i < 3; i++)
	{
		x[i] = Direction[0][i];
		y[i] = Direction[1][i];
		z[i] = Direction[2][i];
	}
}
void DCMGeometry::GetSpacing(double* spacing){
	for (int i = 0; i < 3; i++)
	{
		spacing[i] = InputResolution[i];
	}
}
void DCMGeometry::GetBounds(double* bounds)
{
	for (int i = 0; i < 6; i++)
	{
		bounds[i] = Bound[i];
	}
}

void DCMGeometry::GetExtent(int* extent)
{
	for (int i = 0; i < 6; i++)
	{
		extent[i] = Extent[i];
	}
}
void DCMGeometry::PrintSelf(ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os, indent);
}


