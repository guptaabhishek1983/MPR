#include "Stdafx.h"
#include "MPR_UI_Interface.h"
#include "Utility_Functions.h"

// meddiff includes
#include "rad_util.h"
#include "rad_logger.h"

// VTK icludes
#include "vtkImageData.h"
#include "vtkSmartPointer.h"

using namespace System;
using namespace System::IO;
using namespace MPR_VTK_BRIDGE;
using namespace ImageUtils;

#define __FILENAME__ "MPR_UI_Interface.cpp"
#undef __MODULENAME__
#define __MODULENAME__ "MPR_UI_Interface"

MPR_UI_Interface::MPR_UI_Interface()
{

	if (File::Exists("C:\\Temp\\MPR_View.log"))
	{
		File::Delete("C:\\Temp\\MPR_View.log");
	}
	rad_setLogFileName("C:\\Temp\\MPR_View.log");
	rad_setLogLevel(7);
}

MPR_UI_Interface^ MPR_UI_Interface::GetHandle()
{
	if (m_handle == nullptr)
	{
		m_handle = gcnew MPR_UI_Interface();
	}
	return m_handle;
}
MPR_UI_Interface::~MPR_UI_Interface(void)
{
}

void MPR_UI_Interface::InitMPR(String^ path)
{
	this->m_mpr = new MPR();

	// find all dcm files.
	array<String^>^ dirs = Directory::GetFiles(path, "*.dcm");
	vector<string> fileNames;
	for (int i = 0; i < dirs->Length; i++)
	{
		String^ _strFile = safe_cast<String^>(dirs[i]);
		const char* dicomFile = convert_to_const_charPtr(_strFile);
		fileNames.push_back(dicomFile);
	}
	this->m_mpr->initFromDir1(fileNames);

	long val = this->m_mpr->GetPixelIntensity(Axis::SagittalAxis, 152, 162);
}

BitmapWrapper^ MPR_UI_Interface::GetDisplayImage(int axis)
{

	image displayImage = this->m_mpr->GetOutputImage((Axis)axis);
	
	BitmapWrapper^ bmp = gcnew BitmapWrapper(displayImage.data, displayImage.width, displayImage.height, "MONOCHROME");
	int newWidth, newHeight;
	//this->m_mpr->GetOutputImageDisplayDimensions((Axis)axis, newWidth, newHeight);
	//bmp->Resize(newWidth, newHeight);
	if (m_updateImage != nullptr)
	{
		double xPos = 0, yPos = 0;
		this->m_mpr->GetCurrentSlicerPositionRelativeToIndex((Axis)axis, xPos, yPos);
		EVT_UpdateImage(bmp, axis, xPos, yPos);
	}
	return bmp;
}

int MPR_UI_Interface::GetNumberOfImages(int axis)
{
	return this->m_mpr->GetNumberOfImages((Axis)axis);
}
void MPR_UI_Interface::Scroll(int axis, int delta)
{
	this->m_mpr->Scroll((Axis)axis, delta);
}

int MPR_UI_Interface::GetCurrentImageIndex(int axis)
{
	return this->m_mpr->GetCurrentImageIndex((Axis)axis);
}

double MPR_UI_Interface::GetCurrentImagePosition(int axis)
{
	return this->m_mpr->GetCurrentImagePosition((Axis)axis);
}

void MPR_UI_Interface::GetCurrentSlicerPositionRelativeToIndex(int axis, int* pos)
{
	double xPos = 0, yPos = 0;
	this->m_mpr->GetCurrentSlicerPositionRelativeToIndex((Axis)axis,xPos, yPos);
	pos[0] = xPos;
	pos[1] = yPos;
}

void MPR_UI_Interface::UpdateSlicerPosition(int axis, float x, float y)
{
	this->m_mpr->Scroll2((Axis)axis, x, y);
	BitmapWrapper^ sagittal_bmp = GetDisplayImage((int)Axis::SagittalAxis);
	BitmapWrapper^ coronal_bmp = GetDisplayImage((int)Axis::CoronalAxis);
	BitmapWrapper^ axial_bmp = GetDisplayImage((int)Axis::AxialAxis);

	//switch ((Axis)axis)
	//{
	//	case AxialAxis:
	//	{
	//		BitmapWrapper^ sagittal_bmp = GetDisplayImage((int)Axis::SagittalAxis);
	//		BitmapWrapper^ coronal_bmp = GetDisplayImage((int)Axis::CoronalAxis);
	//		/*if (m_updateImage != nullptr)
	//		{
	//			EVT_UpdateImage(sagittal_bmp, Axis::SagittalAxis, pos[0], pos[1]);
	//			EVT_UpdateImage(coronal_bmp, Axis::CoronalAxis, pos[0], pos[1]);
	//		}*/
	//	}
	//		break;
	//	case SagittalAxis:
	//	{
	//		/*this->m_mpr->Scroll2(Axis::CoronalAxis, x);
	//		this->m_mpr->Scroll2(Axis::AxialAxis, y);*/

	//		BitmapWrapper^ axial_bmp = GetDisplayImage((int)Axis::AxialAxis);
	//		BitmapWrapper^ coronal_bmp = GetDisplayImage((int)Axis::CoronalAxis);
	//		/*if (m_updateImage != nullptr)
	//		{
	//			EVT_UpdateImage(axial_bmp, Axis::AxialAxis, pos[0], pos[1]);
	//			EVT_UpdateImage(coronal_bmp, Axis::CoronalAxis, pos[0], pos[1]);
	//		}*/
	//	}
	//		break;
	//	case CoronalAxis:
	//	{
	//		/*this->m_mpr->Scroll2(Axis::SagittalAxis , x);
	//		this->m_mpr->Scroll2(Axis::AxialAxis, y);*/

	//		BitmapWrapper^ sagittal_bmp = GetDisplayImage((int)Axis::SagittalAxis);
	//		BitmapWrapper^ axial_bmp = GetDisplayImage((int)Axis::AxialAxis);
	//		/*if (m_updateImage != nullptr)
	//		{
	//			EVT_UpdateImage(sagittal_bmp, Axis::SagittalAxis, pos[0], pos[1]);
	//			EVT_UpdateImage(axial_bmp, Axis::AxialAxis, pos[0], pos[1]);
	//		}*/
	//	}
	//		break;
	//	default:
	//		break;
	//}
}

String^ MPR_UI_Interface::GetOrientationMarkerLeft(int axis)
{
	return convert_to_managed_string(this->m_mpr->GetOrientationMarkerLeft((Axis)axis));
}
String^ MPR_UI_Interface::GetOrientationMarkerRight(int axis)
{
	return convert_to_managed_string(this->m_mpr->GetOrientationMarkerRight((Axis)axis));
}
String^ MPR_UI_Interface::GetOrientationMarkerTop(int axis)
{
	return convert_to_managed_string(this->m_mpr->GetOrientationMarkerTop((Axis)axis));
}
String^ MPR_UI_Interface::GetOrientationMarkerBottom(int axis)
{
	return convert_to_managed_string(this->m_mpr->GetOrientationMarkerBottom((Axis)axis));
}


void MPR_UI_Interface::GetPixelSpacing(int p_axis, double* pixelSpacing)
{
	this->m_mpr->GetXYZPixelSpacing(p_axis,pixelSpacing);
}

long int MPR_UI_Interface::GetPixelIntensity(int axis, int x_pos, int y_pos)
{
	if (this->m_mpr == NULL) return 0;
	
	long value = this->m_mpr->GetPixelIntensity((Axis)axis, x_pos, y_pos);
	return value;
}
void MPR_UI_Interface::RotateAxesAlongPlane(int axis, int angle)
{
	this->m_mpr->RotateAxesAlongPlane(axis, angle);
	BitmapWrapper^ sagittal_bmp = GetDisplayImage((int)Axis::SagittalAxis);

	BitmapWrapper^ coronal_bmp = GetDisplayImage((int)Axis::CoronalAxis);
	BitmapWrapper^ axial_bmp = GetDisplayImage((int)Axis::AxialAxis);
	//switch (axis)	
	//{
	//	case AxialAxis:
	//	{
	//		BitmapWrapper^ sagittal_bmp = GetDisplayImage((int)Axis::SagittalAxis);
	//		
	//		BitmapWrapper^ coronal_bmp = GetDisplayImage((int)Axis::CoronalAxis);
	//		/*if (m_updateImage != nullptr)
	//		{
	//			EVT_UpdateImage(sagittal_bmp, Axis::SagittalAxis, 0, 0);
	//			EVT_UpdateImage(coronal_bmp, Axis::CoronalAxis, 0, 0);
	//		}*/
	//	}
	//		break;
	//	case SagittalAxis:
	//	{
	//		BitmapWrapper^ axial_bmp = GetDisplayImage((int)Axis::AxialAxis);
	//		BitmapWrapper^ coronal_bmp = GetDisplayImage((int)Axis::CoronalAxis);
	//		/*if (m_updateImage != nullptr)
	//		{
	//			EVT_UpdateImage(axial_bmp, Axis::AxialAxis, 0, 0);
	//			EVT_UpdateImage(coronal_bmp, Axis::CoronalAxis, 0, 0);
	//		}*/
	//	}
	//		break;
	//	case CoronalAxis:
	//	{
	//		BitmapWrapper^ sagittal_bmp = GetDisplayImage((int)Axis::SagittalAxis);
	//		BitmapWrapper^ axial_bmp = GetDisplayImage((int)Axis::AxialAxis);
	//		/*if (m_updateImage != nullptr)
	//		{
	//			EVT_UpdateImage(sagittal_bmp, Axis::SagittalAxis, 0, 0);
	//			EVT_UpdateImage(axial_bmp, Axis::AxialAxis, 0, 0);
	//		}*/
	//	}
	//		break;
	//	default:
	//		break;
	//}
}

// static methods
void MPR_UI_Interface::WriteLog(String^ msg)
{
	const char* m = convert_to_const_charPtr(msg);
	RAD_LOG_INFO("UI:"<<m);
}