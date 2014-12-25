#pragma once
#include "enums.h"

class vtkImageReslice;
class vtkMatrix4x4;
class vtkImageData;
class vtkTransform;
#include <vtkSmartPointer.h>

namespace RTViewer
{
	class DoseReslicer
	{
	public:
		DoseReslicer(Axis axis);
		~DoseReslicer();

		vtkSmartPointer<vtkImageData> GetOuputImage(float position);
		void SetInput(vtkImageData* image){ m_inputImage = image; }

	private:
		Axis m_axis;
		vtkImageReslice* m_reslice;
		vtkMatrix4x4* m_resliceMatrix;
		vtkImageData* m_inputImage;
		vtkImageData* m_outputImage;
	};
}
