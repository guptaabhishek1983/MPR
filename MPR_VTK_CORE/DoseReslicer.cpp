#include "DoseReslicer.h"
using namespace RTViewer;

// vtk includes
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkImageReslice.h>
#include <vtkMatrix4x4.h>

DoseReslicer::DoseReslicer(Axis axis)
{
	this->m_axis = axis;
	this->m_reslice = vtkImageReslice::New();
	this->m_resliceMatrix = vtkMatrix4x4::New();
	this->m_resliceMatrix->Identity(); // create identity matrix;
	switch (m_axis)
	{
		case AxialAxis:
			this->m_resliceMatrix->DeepCopy(axialElements);
			break;
		case CoronalAxis:
			this->m_resliceMatrix->DeepCopy(coronalElements);

			break;
		case SagittalAxis:
			this->m_resliceMatrix->DeepCopy(sagittalElements);
			break;
	}
	m_outputImage = NULL;
	m_inputImage = NULL;
}


DoseReslicer::~DoseReslicer()
{
	this->m_resliceMatrix->Delete();
	this->m_reslice->Delete();
}

vtkSmartPointer<vtkImageData> DoseReslicer::GetOuputImage(float position)
{
	switch (m_axis)
	{
		case AxialAxis:
		{
			m_resliceMatrix->SetElement(0, 3, 0);
			m_resliceMatrix->SetElement(1, 3, 0);
			m_resliceMatrix->SetElement(2, 3, position);
		}
			break;
		case CoronalAxis:
		{
			m_resliceMatrix->SetElement(0, 3, 0);
			m_resliceMatrix->SetElement(1, 3, position);
			m_resliceMatrix->SetElement(2, 3, 0);
		}
			break;
		case SagittalAxis:
		{
			m_resliceMatrix->SetElement(0, 3, position);
			m_resliceMatrix->SetElement(1, 3, 0);
			m_resliceMatrix->SetElement(2, 3, 0);
		}
			break;
	}
	this->m_reslice->SetResliceAxes(m_resliceMatrix);
	this->m_reslice->SetInputData(this->m_inputImage);
	this->m_reslice->SetOutputDimensionality(2);
	//this->m_reslice->SetInterpolationModeToCubic(); 
	this->m_reslice->Update();
	this->m_outputImage = this->m_reslice->GetOutput();
	return this->m_outputImage;
}
