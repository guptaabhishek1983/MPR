#include "Stdafx.h"
#include "MPR_UI_Interface.h"
#include "Utility_Functions.h"

// meddiff includes
#include "rad_util.h"
#include "rad_logger.h"

// VTK icludes
#include "vtkImageData.h"
#include "vtkSmartPointer.h"
#include "vtkPointData.h"

using namespace System;
using namespace System::IO;
using namespace System::Threading;
using namespace System::Collections::Generic;
using namespace System::Drawing;
using namespace MPR_VTK_BRIDGE;
using namespace ImageUtils;
using namespace EventHandling;

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

	ThreadPool::SetMaxThreads(4, 4);

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

#include "DVHComputation.h"

void MPR_UI_Interface::InitMPR(String^ path)
{
	
	/*RAD_LOG_CRITICAL("Loading structure set");
	RadRTDicomInterface* pDicom = new RTDcmtkDicomInterface("D:\\DicomDataSet\\HCG-Dicoms\\StreamingData\\ESHWAR_GOWDA\\RTSS\\rtss.dcm");

	this->m_rtss = new RTStructureSet((RTDcmtkDicomInterface*)pDicom);

	RAD_LOG_CRITICAL("Loading plan");
	RadRTDicomInterface* planDicom = new RTDcmtkDicomInterface("D:\\DicomDataSet\\HCG-Dicoms\\StreamingData\\ESHWAR_GOWDA\\RTPLAN\\RTPLAN.dcm");

	this->m_plan = new Plan((RTDcmtkDicomInterface*)planDicom);

	RAD_LOG_CRITICAL("Loading dose");
	RadRTDicomInterface* doseDicom = new RTDcmtkDicomInterface("D:\\DicomDataSet\\HCG-Dicoms\\StreamingData\\ESHWAR_GOWDA\\RTDOSE\\RTDOSE.dcm");

	this->m_dose = new Dose((RTDcmtkDicomInterface*)doseDicom, m_plan);
	
	
	*/

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


	double origin[3] = { 0,0,0 };
	this->m_mpr->GetDCMGeometry()->GetOrigin(origin);
	
	double spacing[3] = { 0, 0, 0 };
	this->m_mpr->GetDCMGeometry()->GetSpacing(spacing);

	//input_image->GetOrigin(origin);

	int dim[3] = { 0, 0, 0 };
	this->m_mpr->GetDCMGeometry()->GetDimensions(dim);
	
	double xOrient[3] = { 0, 0, 0 };
	double yOrient[3] = { 0, 0, 0 };
	double zOrient[3] = { 0, 0, 0 };
	this->m_mpr->GetDCMGeometry()->GetOrientation(xOrient, yOrient,zOrient);

	/*double DoseOrigin[3] = { 0, 0, 0 };
	this->m_dose->GetDCMGeometry()->GetOrigin(DoseOrigin);
	double t[3] = { 0, 0, 0 };
	t[0] = fabs(DoseOrigin[0] - origin[0]);
	t[1] = fabs(DoseOrigin[1] - origin[1]);
	t[2] = fabs(DoseOrigin[2] - origin[2]);

	this->m_dose->InitMPRPipeline();*/

	for (int i = 0; i < 3; i++)
	{
		xOrient[i] = fabs(xOrient[i]);
		yOrient[i] = fabs(yOrient[i]);
	}
	cout << "origin:" << origin[0] << "," << origin[1] << "," << origin[2] << endl;
	cout << "spacing:" << spacing[0] << "," << spacing[1] << "," << spacing[2] << endl;
	cout << "xOrient:" << xOrient[0] << "," << xOrient[1] << "," << xOrient[2] << endl;
	cout << "yOrient:" << yOrient[0] << "," << yOrient[1] << "," << yOrient[2] << endl;
	cout << "dim:" << dim[0] << "," << dim[1] << "," << dim[2] << endl;
	//this->m_rtss->ComputeSlicers(origin, spacing, xOrient, yOrient, dim);
	long val = this->m_mpr->GetPixelIntensity(Axis::SagittalAxis, 152, 162);
}
void MPR_UI_Interface::GetCurrentSliceROI(int axis, Dictionary<int, Dictionary<int, List<PointF>^>^>^% roiPoints, Dictionary<int, Color>^% roiColor)
{
	/*vector<vector<vector<ROIPlotPoints>>> outPoints;
	vector<ROIColor> outColors;
	double pos = this->m_mpr->GetCurrentImagePosition((Axis)axis);

	vtkSmartPointer<vtkImageData> input_image = this->m_mpr->GetInputImage();
	double bounds[6] = { 0, 0, 0, 0, 0, 0 };
	input_image->GetBounds(bounds);
	double fip[3] = { bounds[0], bounds[2], bounds[4] };
	double lip[3] = { bounds[0], bounds[2], bounds[5] };
	double origin[3] = { 0, 0, 0 };
	input_image->GetOrigin(origin);

	int dim[3] = { 0, 0, 0 };
	input_image->GetDimensions(dim);

	double xOrient[3] = { 0, 0, 0 };
	double yOrient[3] = { 0, 0, 0 };
	this->m_mpr->GetOrientation(xOrient, yOrient);
	vtkSmartPointer<vtkTransform> transform = this->m_mpr->GetTransform((Axis)axis);
	this->m_rtss->ComputeROI(axis, pos, fip, lip, xOrient, yOrient, outPoints, outColors, transform);

	
	for (int outColorsIdx = 0; outColorsIdx<outColors.size(); outColorsIdx++)
	{
		roiColor->Add(outColors.at(outColorsIdx).roiNumber, Color::FromArgb(outColors.at(outColorsIdx).r, outColors.at(outColorsIdx).g, outColors.at(outColorsIdx).b));
	}

	
	roiPoints->Clear();
	for (int i = 0; i < outPoints.size(); i++)
	{
		for (int cellIdx = 0; cellIdx < outPoints.at(i).size(); cellIdx++)
		{
			for (int j = 0; j < outPoints.at(i).at(cellIdx).size(); j++)
			{
				float x = outPoints.at(i).at(cellIdx).at(j).x;
				float y = outPoints.at(i).at(cellIdx).at(j).y;
				float z = outPoints.at(i).at(cellIdx).at(j).z;
				int roiNumber = outPoints.at(i).at(cellIdx).at(j).roiNumber;
				if (roiPoints->ContainsKey(roiNumber) == false)
				{
					roiPoints->Add(roiNumber, gcnew Dictionary<int, List<PointF>^>());
				}
				Dictionary<int, List<PointF>^ >^ contourLineSegment = roiPoints[roiNumber];

				if (!contourLineSegment->ContainsKey(cellIdx))
				{
					contourLineSegment->Add(cellIdx, gcnew List<PointF>());
				}
				List<PointF>^ roiPointList = contourLineSegment[cellIdx];
				PointF p(x, y);
				roiPointList->Add(p);
			}
		}
	}*/
	return;
}

#include "vtkMath.h"
BitmapWrapper^ MPR_UI_Interface::GetDisplayImage(int axis)
{
	rad_timer t1;
	t1.start();

	image displayImage = this->m_mpr->GetOutputImage((Axis)axis);
	t1.end();
	RAD_LOG_CRITICAL("Getting output image for Axis:" << axis << " took:" << t1.getIntervalMilliseconds() << " ms.");
	t1.start();
	BitmapWrapper^ bmp = gcnew BitmapWrapper(displayImage.data, displayImage.width, displayImage.height, "MONOCHROME");
	t1.end();
	RAD_LOG_CRITICAL("Preparing bitmap for Axis:" << axis << " took:" << t1.getIntervalMilliseconds() << " ms.");
	int newWidth, newHeight;
	//this->m_mpr->GetOutputImageDisplayDimensions((Axis)axis, newWidth, newHeight);
	//bmp->Resize(newWidth, newHeight);

	
	//if (m_updateImage != nullptr)
	//{
	
	//-- compute ROI
	Dictionary<int, Dictionary<int, List<PointF>^>^>^ roiPoints = gcnew Dictionary<int, Dictionary<int, List<PointF>^>^>();
	Dictionary<int, Color>^% roiColor = gcnew Dictionary<int, Color>();

	//vector<vector<vector<ROIPlotPoints>>> outPoints;
	//vector<ROIColor> outColors;
	//
	////int idx = this->m_mpr->GetCurrentImageIndex((Axis)axis);
	//double pos = this->m_mpr->GetCurrentImagePosition((Axis)axis);
	//
	//double fip[3] = { 0,0,0 };
	//double lip[3] = { 0,0,0};
	//
	//this->m_mpr->GetDCMGeometry()->GetOrigin(fip);
	//this->m_mpr->GetDCMGeometry()->GetLastImagePosition(lip);
	//if (axis == (int)Axis::AxialAxis)// || axis == (int)Axis::SagittalAxis)
	//{
	//	this->m_rtss->ComputeROI(axis, pos, fip, lip, outPoints, outColors);
	//}

	//for (int outColorsIdx = 0; outColorsIdx<outColors.size(); outColorsIdx++)
	//{
	//	roiColor->Add(outColors.at(outColorsIdx).roiNumber, Color::FromArgb(outColors.at(outColorsIdx).r, outColors.at(outColorsIdx).g, outColors.at(outColorsIdx).b));
	//}


	//roiPoints->Clear();
	//for (int i = 0; i < outPoints.size(); i++)
	//{
	//	for (int cellIdx = 0; cellIdx < outPoints.at(i).size(); cellIdx++)
	//	{
	//		for (int j = 0; j < outPoints.at(i).at(cellIdx).size(); j++)
	//		{
	//			float x = outPoints.at(i).at(cellIdx).at(j).x;
	//			float y = outPoints.at(i).at(cellIdx).at(j).y;
	//			float z = outPoints.at(i).at(cellIdx).at(j).z;
	//			double inPos[3] = { x, y, z };
	//			double outPos[3] = { 0, 0, 0 };
	//			//this->m_mpr->TransformPoint((Axis)axis, inPos, outPos);
	//			/*x = outPos[0];
	//			y = outPos[0];
	//			z = outPos[0];*/
	//			int roiNumber = outPoints.at(i).at(cellIdx).at(j).roiNumber;
	//			if (roiPoints->ContainsKey(roiNumber) == false)
	//			{
	//				roiPoints->Add(roiNumber, gcnew Dictionary<int, List<PointF>^>());
	//			}
	//			Dictionary<int, List<PointF>^ >^ contourLineSegment = roiPoints[roiNumber];

	//			if (!contourLineSegment->ContainsKey(cellIdx))
	//			{
	//				contourLineSegment->Add(cellIdx, gcnew List<PointF>());
	//			}
	//			List<PointF>^ roiPointList = contourLineSegment[cellIdx];
	//			PointF p(x, y);
	//			roiPointList->Add(p);
	//		}
	//	}
	//}

	////-- compute DOSE colorwash & isodose curve.
	Dictionary<int, Dictionary<int, List<PointF>^>^>^ dosePoints = gcnew Dictionary<int, Dictionary<int, List<PointF>^>^>();
	Dictionary<int, Color>^% doseColor = gcnew Dictionary<int, Color>();

	BitmapWrapper^ dose_bmp = nullptr;
	double tx = 0, ty = 0;
	//// --- Get CT image position 
	//if (m_dose !=NULL)
	//{
	//	pos = this->m_mpr->GetCurrentImagePosition((Axis)axis);

	//	// --- check if position is in bounds of dose;
	//	if (this->m_dose->GetDCMGeometry()->CheckBounds(pos, (Axis)axis))
	//	{
	//		int dose_idx = this->m_dose->GetDCMGeometry()->ComputeIndex(pos, (Axis)axis);


	//		
	//		//pos = this->m_mpr->GetSlicerPosition((Axis)axis);
	//		RAD_LOG_CRITICAL("Axis:" << axis << "; DOSE IDX:" << dose_idx << "; POS:"<<pos);
	//		

	//		vtkSmartPointer<vtkImageData>doseColorImage = NULL;// this->m_dose->ComputeColorWash2(axis, dose_idx, tx, ty);

	//		
	//		//this->m_dose->ComputeColorWash2(axis, dose_idx, tx, ty);

	//		double targetPlaneSpacing[3] = { 0, 0, 0 };
	//		this->m_mpr->GetDCMGeometry()->GetSpacing(targetPlaneSpacing);


	//		double CTOrigin[3] = { 0, 0, 0 };
	//		this->m_mpr->GetDCMGeometry()->GetOrigin(CTOrigin);

	//		double DOSE_FIP[3] = { 0, 0, 0 };
	//		double DOSE_LIP[3] = { 0, 0, 0 };
	//		this->m_dose->GetDCMGeometry()->GetOrigin(DOSE_FIP);
	//		this->m_dose->GetDCMGeometry()->GetLastImagePosition(DOSE_LIP);

	//		double DoseOrigin[3] = { 0, 0, 0 };
	//		if (CTOrigin[2] == DOSE_FIP[2])
	//		{
	//			DoseOrigin[0] = DOSE_FIP[0];
	//			DoseOrigin[1] = DOSE_FIP[1];
	//			DoseOrigin[2] = DOSE_FIP[2];

	//		}
	//		else if (CTOrigin[2] == DOSE_LIP[2])
	//		{
	//			DoseOrigin[0] = DOSE_LIP[0];
	//			DoseOrigin[1] = DOSE_LIP[1];
	//			DoseOrigin[2] = DOSE_LIP[2];

	//		}
	//		else
	//		{
	//			DoseOrigin[0] = DOSE_LIP[0];
	//			DoseOrigin[1] = DOSE_LIP[1];
	//			DoseOrigin[2] = DOSE_LIP[2];
	//		}

	//		// for hfp data
	//		//this->m_dose->GetDCMGeometry()->GetLastImagePosition(DoseOrigin);

	//		// for non
	//		double spacing[3] = { 0, 0, 0 };
	//		this->m_dose->GetDCMGeometry()->GetSpacing(spacing);

	//		for (int i = 0; i < 3; i++)
	//		{
	//			targetPlaneSpacing[i] = fabs(targetPlaneSpacing[i]);
	//			spacing[i] = fabs(spacing[i]);
	//		}


	//		if (doseColorImage != NULL)
	//		{


	//			int colorWashImageDim[3];
	//			doseColorImage->GetDimensions(colorWashImageDim);
	//			void* pData = doseColorImage->GetScalarPointer();

	//			dose_bmp = gcnew BitmapWrapper(doseColorImage->GetScalarPointer(), colorWashImageDim[0], colorWashImageDim[1], "RGB");
	//			dose_bmp->ChangeImageOpacity(0.5);


	//			double sx = 1, sy = 1;


	//			if (axis == 0)
	//			{
	//				sx = sx* (spacing[0] / targetPlaneSpacing[0]);
	//				sy = sy* (spacing[1] / targetPlaneSpacing[1]);
	//				tx = fabs(CTOrigin[0] - DoseOrigin[0]);
	//				ty = fabs(CTOrigin[1] - DoseOrigin[1]);
	//			}
	//			else if (axis == 1)
	//			{
	//				sx = spacing[0] / targetPlaneSpacing[0];
	//				sy = spacing[2] / targetPlaneSpacing[1];
	//				tx = fabs(CTOrigin[0] - DoseOrigin[0]);
	//				ty = fabs(CTOrigin[2] - DoseOrigin[2]);
	//			}
	//			else if (axis == 2)
	//			{
	//				sx = sx* (spacing[1] / targetPlaneSpacing[0]);
	//				sy = sy* (spacing[2] / targetPlaneSpacing[1]);

	//				tx = fabs(CTOrigin[1] - DoseOrigin[1]);
	//				ty = fabs(CTOrigin[2] - DoseOrigin[2]);


	//			}

	//			tx = tx / targetPlaneSpacing[0];
	//			ty = ty / targetPlaneSpacing[1];
	//			sx = fabs(sx);
	//			sy = fabs(sy);
	//			dose_bmp->Resize(colorWashImageDim[0] * sx, colorWashImageDim[1] * sy);

	//		}

	//		// -- compute dose curve
	//		double fip1[3] = { 0, 0, 0 };
	//		double lip1[3] = { 0, 0, 0 };

	//		this->m_dose->GetDCMGeometry()->GetOrigin(fip1);
	//		this->m_dose->GetDCMGeometry()->GetLastImagePosition(lip1);
	//		vector<vector<vector<dosePlotPoint>> > outPoints;
	//		//this->m_dose->ComputeROI(axis, dose_idx, targetPlaneSpacing, lip, outPoints);

	//		if (outPoints.size() > 0)
	//		{
	//			RAD_LOG_INFO("ISODOSE CURVE PRESENT FOR AXIS:" << axis);
	//			//
	//			for (int i = outPoints.size() - 1; i >= 0; i--)
	//			{
	//				for (int cellIdx = 0; cellIdx < outPoints.at(i).size(); cellIdx++)
	//				{
	//					for (int j = 0; j < outPoints.at(i).at(cellIdx).size(); j++)
	//					{
	//						int doseLevel = outPoints.at(i).at(cellIdx).at(j).level;
	//						if (dosePoints->ContainsKey(doseLevel) == false)
	//						{
	//							dosePoints->Add(doseLevel, gcnew Dictionary<int, List<PointF>^>());
	//						}

	//						Dictionary<int, List<PointF>^ >^ isodoseLineSegment = dosePoints[doseLevel];

	//						if (isodoseLineSegment->ContainsKey(cellIdx) == false)
	//						{
	//							isodoseLineSegment->Add(cellIdx, gcnew List<PointF>());
	//						}
	//						List<PointF>^ dosePointList = isodoseLineSegment[cellIdx];
	//						float x = (outPoints.at(i).at(cellIdx).at(j).x)+tx *targetPlaneSpacing[0];
	//						float y = (outPoints.at(i).at(cellIdx).at(j).y)+ty *targetPlaneSpacing[1];
	//						PointF p(x, y);
	//						dosePointList->Add(p);

	//						if (j == 0)
	//						{
	//							int color[3] = { 255, 0, 0 };
	//							if (doseColor->ContainsKey(doseLevel) == false)
	//							{
	//								doseColor->Add(doseLevel, Color::FromArgb(color[0], color[1], color[2]));
	//							}
	//							// TODO: Delete this event.
	//							//EVT_AddColorIsoDoseCollectionMPR(axis, outPointsFromMPR.at(i).at(cellIdx).at(j).level , seriesUID, color[0],color[1], color[2]);
	//						}
	//					}
	//				}
	//			}
	//		}

	//		
	//	}
	//}
	double xPos = 0, yPos = 0;
		this->m_mpr->GetCurrentSlicerPositionRelativeToIndex((Axis)axis, xPos, yPos);
		EventHandler1::Instance->RaiseUpdateImage(bmp, axis, xPos, yPos, roiPoints, roiColor, dose_bmp,tx,ty, dosePoints, doseColor);
	//	this->EVT_UpdateImage(bmp, axis, xPos, yPos);
	//	//ThreadPool::QueueUserWorkItem(gcnew WaitCallback(EVT_UpdateImage));
	//	//EVT_UpdateImage();
	//}
	return bmp;
}

int MPR_UI_Interface::GetNumberOfImages(int axis)
{
	return this->m_mpr->GetNumberOfImages((Axis)axis);
}
void MPR_UI_Interface::Scroll(int axis, int delta)
{
	this->m_mpr->Scroll((Axis)axis, delta);
	//double pos = this->m_mpr->GetCurrentImagePosition((Axis)axis);

	//// --- check if position is in bounds of dose;
	//if (this->m_dose->GetDCMGeometry()->CheckBounds(pos, (Axis)axis))
	//{
	//	int dose_idx = this->m_dose->GetDCMGeometry()->ComputeIndex(pos, (Axis)axis);

	//	//RAD_LOG_CRITICAL("DOSE IDX:" << dose_idx);

	//	//this->m_dose->Scroll((Axis)axis, dose_idx);
	//}
	{BitmapWrapper^ bmp = GetDisplayImage(AxialAxis); }
	{BitmapWrapper^ bmp = GetDisplayImage(SagittalAxis); }
	{BitmapWrapper^ bmp = GetDisplayImage(CoronalAxis); }
}

int MPR_UI_Interface::GetCurrentImageIndex(int axis)
{
	return this->m_mpr->GetCurrentImageIndex((Axis)axis);
}

double MPR_UI_Interface::GetCurrentImagePosition(int axis)
{
	return this->m_mpr->GetCurrentImagePosition((Axis)axis);
}

int MPR_UI_Interface::GetCurrentDoseImageIndex(int axis)
{
	double pos = this->m_mpr->GetCurrentImagePosition((Axis)axis);

	// --- check if position is in bounds of dose;
	if (this->m_dose->GetDCMGeometry()->CheckBounds(pos, (Axis)axis))
	{
		return this->m_dose->GetDCMGeometry()->ComputeIndex(pos, (Axis)axis);
	}

	return VTK_INT_MAX;
}

double MPR_UI_Interface::GetCurrentDoseImagePosition(int axis)
{
	return this->m_dose->GetCurrentImagePosition((Axis)axis);
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
	//this->m_dose->Scroll2((Axis)axis, x, y);
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

void MPR_UI_Interface::GetDVHData(Dictionary<int, List<double>^>^% dvhPoints, Dictionary<int, List<double>^>^% computed_dvhPoints)
{

	this->m_dose->computed_dvh.clear();
	this->m_dose->ComputeDVH(this->m_rtss->getStructureSetROISequences(1));
	this->m_dose->ComputeDVH(this->m_rtss->getStructureSetROISequences(2));

	//for (int i = 0; i < this->m_rtss->getStructureSetROISequences().size(); i++)
	//{
	//}

	for (int i = 0; i < this->m_dose->dh.size(); i++)
	{
		if (this->m_dose->dh.at(i)->dvhDataList.size())
		{
			array<double>^ ret = gcnew array<double>(this->m_dose->dh.at(i)->dvhDataList.size());

			pin_ptr<double> dest = &ret[0];
			std::memcpy(dest, &this->m_dose->dh.at(i)->dvhDataList[0], this->m_dose->dh.at(i)->dvhDataList.size() * sizeof(double));
			
			dvhPoints->Add(this->m_dose->dh.at(i)->getROINumber(), gcnew List<double>(ret));
		}
	}
	for (int i = 0; i < this->m_dose->computed_dvh.size(); i++)
	{
		if (this->m_dose->computed_dvh.at(i)->dvhDataList.size())
		{
			array<double>^ ret = gcnew array<double>(this->m_dose->computed_dvh.at(i)->dvhDataList.size());

			pin_ptr<double> dest = &ret[0];
			std::memcpy(dest, &this->m_dose->computed_dvh.at(i)->dvhDataList[0], this->m_dose->computed_dvh.at(i)->dvhDataList.size() * sizeof(double));

			computed_dvhPoints->Add(this->m_dose->computed_dvh.at(i)->getROINumber(), gcnew List<double>(ret));
		}
	}
	
	return;
}
int MPR_UI_Interface::GetTragetPrescribedDose()
{
	return this->m_dose->rxDose;

}