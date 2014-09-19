
#include "MPR.h"
#include "MPRSlicer.h"

// meddiff includes
#include "rad_util.h"
#include "rad_logger.h"
#include "RTDcmtkDicomInterface.h"
#include "rad_template_helper.h"

//dcmtk includes

// VTK includes
#include "vtkImageData.h"
#include "vtkMatrix4x4.h"
#include "vtkSmartPointer.h"
#include "vtkDicomImageReader.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include <vtkUnsignedIntArray.h>
#include <vtkSignedCharArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedShortArray.h>
#include <vtkShortArray.h>
#include <vtkIntArray.h>

using namespace RTViewer;

#define __FILENAME__ "MPR.cpp"
#undef __MODULENAME__
#define __MODULENAME__ "MPR"

namespace RTViewer
{
	struct MPRData
	{
	private:
		vtkSmartPointer<vtkImageData> m_inputImage;
		
	public:
		MPRSlicer* m_slicers[3];
		double m_lastImagePosition[3];
		double m_imageOrientation[6];
		double m_normal[3];
		double m_matrx[16];
		vtkSmartPointer<vtkMatrix4x4> m_initOrient;
		double m_ww;
		double m_wl;
		double m_rs;
		double m_ri;
	private:
		
	public:
		MPRSlicer* GetSlicer(Axis axis)
		{
			return m_slicers[axis];
		}
		void SetInput(vtkSmartPointer<vtkImageData> image) { this->m_inputImage = image; }
		vtkSmartPointer<vtkImageData> GetInput() { return this->m_inputImage; }
		
	};
}
MPR::MPR(void)
{
	
	d = new MPRData();
	d->m_initOrient = vtkSmartPointer<vtkMatrix4x4>::New();
	d->m_initOrient->Identity();

}

MPR::~MPR(void)
{
	for (int i = 0; i<3; i++)
	{
		delete d->m_slicers[i];
	}
	delete d;
}

void MPR::initFromDir1(vector<string> dicomFiles)
{
	// load data from scratch.
	map<float, string> sortedDicomFiles;
	map<float, string>::iterator rit;
	
	for (int i = 0; i < dicomFiles.size(); i++)
	{
		RadRTDicomInterface* pDicom = new RTDcmtkDicomInterface(dicomFiles.at(i).c_str());
		if (!pDicom->IsFileLoadedSuccessfully())
		{
			delete pDicom;
			continue;
		}
		if (!((RTDcmtkDicomInterface*)pDicom)->checkIfRTObject())
		{
			string imagePosition = "";
			vector<string> temp;
			imagePosition = pDicom->Get_IMAGE_POSITION();
			tokenize(imagePosition, temp, "\\");
			sortedDicomFiles.insert(std::pair<float, std::string>(convert_to_double(temp[2].c_str()), dicomFiles.at(i)));
		}
		delete pDicom;
	}

	RadDataType dicomDataType = TYPE_NOT_SET;
	void* dicomData;
	double firstImagePosition[3];
	int dimensions[3];
	double spacing[3];
	int dicomDataIdx;
	int dicomSliceSize;
	int dicomDataSize;
	int j = 0;
	for (rit = sortedDicomFiles.begin(); rit != sortedDicomFiles.end(); ++rit)
	{
		
		const char* dicomFile = rit->second.c_str();
		RadRTDicomInterface* pDicom = new RTDcmtkDicomInterface(dicomFile);
		if (j == sortedDicomFiles.size()-1)
		{
			string imagePosition = string(pDicom->Get_IMAGE_POSITION());
			vector<string> _imgPosition;
			tokenize(imagePosition, _imgPosition, "\\", true);
			for (int i = 0; i<_imgPosition.size(); i++)
			{
				firstImagePosition[i] = convert_to_double(_imgPosition.at(i).c_str());
			}

		}
		if (j == 0)
		{
			string imagePosition = string(pDicom->Get_IMAGE_POSITION());
			vector<string> _imgPosition;
			tokenize(imagePosition, _imgPosition, "\\", true);
			for (int i = 0; i<_imgPosition.size(); i++)
			{
				d->m_lastImagePosition[i] = convert_to_double(_imgPosition.at(i).c_str());
				firstImagePosition[i] = convert_to_double(_imgPosition.at(i).c_str());
			}
			
			string imageOrientation = string(pDicom->Get_IMAGE_ORIENTATION());
			/*m_normal[3] = {0,0,0};
			m_imageOrientation[6] = {0,0,0,0,0,0};*/

			vector<string> _imgOrient;
			tokenize(imageOrientation, _imgOrient, "\\", true);
			for (int i = 0; i<_imgOrient.size(); i++)
			{
				d->m_imageOrientation[i] = convert_to_double(_imgOrient.at(i).c_str());
			}
			d->m_normal[0] = (d->m_imageOrientation[1] * d->m_imageOrientation[5]) - (d->m_imageOrientation[2] * d->m_imageOrientation[4]);
			d->m_normal[1] = (d->m_imageOrientation[0] * d->m_imageOrientation[5]) - (d->m_imageOrientation[2] * d->m_imageOrientation[3]);
			d->m_normal[2] = (d->m_imageOrientation[0] * d->m_imageOrientation[4]) - (d->m_imageOrientation[1] * d->m_imageOrientation[3]);

			d->m_matrx[0] = d->m_imageOrientation[0];
			d->m_matrx[1] = d->m_imageOrientation[1];
			d->m_matrx[2] = d->m_imageOrientation[2];
			d->m_matrx[3] = 0;

			d->m_matrx[4] = d->m_imageOrientation[3];
			d->m_matrx[5] = d->m_imageOrientation[4];
			d->m_matrx[6] = d->m_imageOrientation[5];
			d->m_matrx[7] = 0;

			d->m_matrx[8] = d->m_normal[0];
			d->m_matrx[9] = d->m_normal[1];
			d->m_matrx[10] = d->m_normal[2];
			d->m_matrx[11] = 0;

			d->m_matrx[12] = 0;
			d->m_matrx[13] = 0;
			d->m_matrx[14] = 0;
			d->m_matrx[15] = 1;

			d->m_initOrient->Identity();
			d->m_initOrient->DeepCopy(d->m_matrx);
			d->m_initOrient->Transpose();
			
			if (pDicom->Get_BITS_ALLOCATED() / 8 == 1)
			{
				if (!strcmp(pDicom->Get_PHOTOMETRIC_INTERPRETATION(), "RGB") ||
					!strcmp(pDicom->Get_PHOTOMETRIC_INTERPRETATION(), "PALETTE COLOR"))
					dicomDataType = TYPE_U32Data;
				else
					dicomDataType = TYPE_U8Data;
			}
			else
			{
				if (!pDicom->Get_PIXEL_REPRESENTATION())
					dicomDataType = TYPE_U16Data;
				else
					dicomDataType = TYPE_S16Data;
			}
			dimensions[0] = pDicom->Get_COLOUMN();
			dimensions[1] = pDicom->Get_ROW();
			dimensions[2] = sortedDicomFiles.size();
			dicomDataSize = dimensions[0] * dimensions[1] * dimensions[2];
			dicomSliceSize = dimensions[0] * dimensions[1];

			string pixelSpacing = string(pDicom->Get_PIXEL_SPACING());
			vector<string> _pixelSpacing;
			tokenize(pixelSpacing, _pixelSpacing, "\\", true);

			spacing[0] = convert_to_double(_pixelSpacing.at(0).c_str());
			spacing[1] = convert_to_double(_pixelSpacing.at(1).c_str());
			spacing[2] = firstImagePosition[2];

			d->m_wl = convert_to_double(pDicom->Get_WINDOW_CENTER());
			d->m_ww = convert_to_double(pDicom->Get_WINDOW_WIDTH());
			d->m_rs = convert_to_double(pDicom->Get_RESCALE_SLOPE());
			d->m_ri = convert_to_double(pDicom->Get_RESCALE_INTERCEPT());
			RAD_LOG_CRITICAL(dicomDataType);
			switch (dicomDataType)
			{
				case TYPE_U32Data:
					dicomData = rad_get_memory(dicomDataSize * rad_sizeof(TYPE_U32Data));
					break;
				case TYPE_U8Data:
					dicomData = rad_get_memory(dicomDataSize * rad_sizeof(TYPE_U8Data));
					break;
				case TYPE_U16Data:
					dicomData = rad_get_memory(dicomDataSize * rad_sizeof(TYPE_U16Data));
					break;
				case TYPE_S16Data:
					dicomData = rad_get_memory(dicomDataSize * rad_sizeof(TYPE_S16Data));
					break;
			}

			dicomDataIdx = dicomSliceSize;
		}
		if (j == 1)
		{
			string imagePosition = string(pDicom->Get_IMAGE_POSITION());
			vector<string> _imgPosition;
			tokenize(imagePosition, _imgPosition, "\\", true);
			spacing[2] = spacing[2] - convert_to_double(_imgPosition.at(2).c_str()); //4.0;
		}

		image pixelData = born_image();
		image overlayPixelData = born_image();
		pDicom->InflateSingleFrameDicomPixelData(&pixelData, &overlayPixelData);
		void* raw_dcm_data = pixelData.data;

		switch (dicomDataType)
		{
			case TYPE_U32Data: {
				U32DataType* dicomData2 = static_cast<U32DataType*>(dicomData);
				dicomData2 += dicomDataSize;
				memcpy(dicomData2 - dicomDataIdx, raw_dcm_data, dicomSliceSize*sizeof(U32DataType));
			} break;
			case TYPE_U8Data: {
				U8DataType* dicomData2 = static_cast<U8DataType*>(dicomData);
				dicomData2 += dicomDataSize;
				memcpy(dicomData2 - dicomDataIdx, raw_dcm_data, dicomSliceSize*sizeof(U8DataType));
			} break;
			case TYPE_U16Data: {
				U16DataType* dicomData2 = static_cast<U16DataType*>(dicomData);
				dicomData2 += dicomDataSize;
				memcpy(dicomData2 - dicomDataIdx, raw_dcm_data, dicomSliceSize*sizeof(U16DataType));
			} break;
			case TYPE_S16Data: {
				S16DataType* dicomData2 = static_cast<S16DataType*>(dicomData);
				dicomData2 += dicomDataSize;
				memcpy(dicomData2 - dicomDataIdx, raw_dcm_data, dicomSliceSize*sizeof(S16DataType));
			} break;
		}
		dicomDataIdx += dicomSliceSize;

		if (pixelData.data)
		{
			rad_free_memory(pixelData.data);
		}
		if (overlayPixelData.data)
		{
			rad_free_memory(overlayPixelData.data);
		}

		delete pDicom;
		j++;
	}
	dicomDataIdx -= dicomSliceSize;
	vtkDataArray* scalars = 0;

	switch (dicomDataType)
	{
		case TYPE_U32Data:
			scalars = vtkUnsignedIntArray::New();
			((vtkUnsignedIntArray*)(scalars))->SetArray((unsigned int*)dicomData, dicomDataSize, 1);
			break;
		case TYPE_U8Data:
			scalars = vtkUnsignedCharArray::New();
			((vtkUnsignedCharArray*)(scalars))->SetArray((unsigned char*)dicomData, dicomDataSize, 1);
			break;
		case TYPE_U16Data:
			scalars = vtkUnsignedShortArray::New();
			((vtkUnsignedShortArray*)(scalars))->SetArray((unsigned short*)dicomData, dicomDataSize, 1);
			break;
		case TYPE_S16Data:
			scalars = vtkShortArray::New();
			((vtkShortArray*)(scalars))->SetArray((short*)dicomData, dicomDataSize, 1);
			break;
	}
	scalars->SetNumberOfComponents(1);

	vtkSmartPointer<vtkImageData> CTMPRCuboid = vtkSmartPointer<vtkImageData>::New();
	//vtkImageData* CTMPRCuboid = vtkImageData::New();
	CTMPRCuboid->SetDimensions(dimensions);
	CTMPRCuboid->SetSpacing(spacing);
	CTMPRCuboid->SetOrigin(firstImagePosition);
	CTMPRCuboid->GetPointData()->SetScalars(scalars);
	CTMPRCuboid->GetPointData()->GetScalars()->SetName("CT Cuboid");

	double CTBounds[6];
	CTMPRCuboid->GetBounds(CTBounds);
	//CTMPRCuboid->SetOrigin(CTBounds[0],CTBounds[2],CTBounds);
	RAD_LOG_CRITICAL("<abhishek> bounds:" << CTBounds[0] << ":" << CTBounds[1] << ":" << CTBounds[2] << ":" << CTBounds[3] << ":" << CTBounds[4] << ":" << CTBounds[5]);
	this->initFromImage(CTMPRCuboid);

	CTMPRCuboid->Print(cerr);
}
void MPR::initFromDir(string dirPath)
{
	vtkSmartPointer<vtkDICOMImageReader> dicomReader  = vtkSmartPointer<vtkDICOMImageReader>::New();
	dicomReader->SetDirectoryName(dirPath.c_str());
	dicomReader->FileLowerLeftOn();
	dicomReader->Update();
	this->initFromImage(dicomReader->GetOutput());
}

void MPR::initFromImage(vtkSmartPointer<vtkImageData> image)
{
	if( d->GetInput()!=NULL )
	{
		d->GetInput()->Delete();
		d->SetInput(NULL);
	}
	d->SetInput(image);
	for(int i=0;i<3;i++)
	{
		MPRSlicer* slicer = new MPRSlicer((Axis)i);
		slicer->SetInput(image);
		slicer->InitSlicer(d->m_initOrient);
		slicer->SetVOI_LUTParameters(d->m_ww, d->m_wl, d->m_rs, d->m_ri);
		//slicer->InitSlicer();

		d->m_slicers[i] = slicer;
	}

	// scroll slicers to get middle image
	for (int i = 0; i<3; i++)
	{
		d->m_slicers[i]->Scroll((d->m_slicers[i]->GetNumberOfImages() / 2));
	}
}

image MPR::GetOutputImage(Axis axis)
{
	for(int i=0;i<3;i++)
	{
		if(i==(int)axis)
		{
			RAD_LOG_INFO("i:" << i);
			return d->m_slicers[i]->GetOutputImage();
		}
	}
	return ::born_image();
}

void MPR::Scroll(Axis axis, int delta)
{
	for(int i=0; i<3; i++)
	{
		if(i == axis)
		{
			d->m_slicers[i]->Scroll(delta);
		}
	}
}

void MPR::Scroll2(Axis axis, float newPosition)
{
	
	double origin[3];
	d->GetInput()->GetOrigin(origin);
	double spacing[3];
	d->GetInput()->GetSpacing(spacing);

	double currentPos = d->m_slicers[axis]->GetSlicerPosition();
	int delta = 0;
	switch (axis)
	{
		case AxialAxis:
		{
			// this condition is valid for Axial plane where z-directions increases in -ve direction. 
			// But UI gives the point in +ve 
			if (spacing[2] < 0)
				newPosition = newPosition*-1;
			newPosition += origin[2];
			delta = (newPosition - currentPos) / spacing[2];
		}
			break;
		case CoronalAxis:
		{
			newPosition += origin[1];
			delta = (newPosition - currentPos) / spacing[1];
		}
			break;
		case SagittalAxis:
		{
			newPosition += origin[0];
			delta = (newPosition - currentPos) / spacing[0];
		}
			break;
		default:
			break;
	}
	this->Scroll(axis, delta);
	RAD_LOG_INFO("Delta is:" << delta);
	return;
}
int MPR::GetNumberOfImages(Axis axis)
{
	int numberOfImages = 0;
	for (int i = 0; i<3; i++)
	{
		if (i == axis)
		{
			numberOfImages = d->m_slicers[i]->GetNumberOfImages();
		}
	}
	return numberOfImages;
}

int MPR::GetCurrentImageIndex(Axis axis)
{
	int idx = 0;
	for (int i = 0; i<3; i++)
	{
		if (i == axis)
		{
			idx = d->m_slicers[i]->GetSlicerPositionAsIndex();
		}
	}
	return idx;
}

double MPR::GetCurrentImagePosition(Axis axis)
{
	double pos = 0;
	for (int i = 0; i<3; i++)
	{
		if (i == axis)
		{
			pos = d->m_slicers[i]->GetSlicerPosition();
		}
	}
	return pos;
}

double MPR::GetCurrentImagePositionRelativeToOrigin(Axis axis)
{
	double spacing[3] = { 0, 0, 0 };
	d->GetInput()->GetSpacing(spacing);

	double pos = 0;
	for (int i = 0; i<3; i++)
	{
		if (i == axis)
		{
			pos = d->m_slicers[i]->GetSlicerPosition();
		}
	}
	
	switch (axis)
	{
		case AxialAxis:
			RAD_LOG_CRITICAL("Axial Slicer position:" << pos);
			break;

		case CoronalAxis:
			RAD_LOG_CRITICAL("Coronal Slicer position:" << pos);

			break;

		case SagittalAxis:
			RAD_LOG_CRITICAL("Sagittal Slicer position:" << pos);
			break;
	}
	double origin[3];
	d->GetInput()->GetOrigin(origin);
	switch (axis)
	{
		case RTViewer::AxialAxis:
			pos -= origin[2];
			break;
		case RTViewer::CoronalAxis:
			pos -= origin[1];
			break;
		case RTViewer::SagittalAxis:
			pos -= origin[0];
			break;
		default:
			break;
	}

	//switch (axis)
	//{
	//	case RTViewer::AxialAxis:
	//		//pos = pos / fabs(spacing[2]);
	//		break;
	//	case RTViewer::CoronalAxis:
	//		pos = pos / fabs(spacing[1]);
	//		break;
	//	case RTViewer::SagittalAxis:
	//		pos = pos / fabs(spacing[0]);
	//		break;
	//	default:
	//		break;
	//}
	return fabs(pos);
}

void MPR::GetOutputImageDisplayDimensions(Axis axis, int& width, int& height)
{
	double spacing[3] = { 0, 0, 0 };
	d->GetInput()->GetSpacing(spacing);
	spacing[0] = fabs(spacing[0]);
	spacing[1] = fabs(spacing[1]);
	spacing[2] = fabs(spacing[2]);
	int dim[3] = { 0, 0, 0 };
	d->GetInput()->GetDimensions(dim);
	switch (axis)
	{
		case RTViewer::AxialAxis:
		{
			width = dim[0] * spacing[0];
			height = dim[1] * spacing[1];
		}
			break;
		case RTViewer::CoronalAxis:
		{
			width = dim[0] * spacing[0];
			height = dim[2] * spacing[2];
		}
			break;
		case RTViewer::SagittalAxis:
		{
			width = dim[1] * spacing[1];
			height = dim[2] * spacing[2];
		}
			break;
		default:
			break;
	}
}


string MPR::GetOrientationMarkerLeft(Axis axis)
{
	for (int i = 0; i<3; i++)
	{
		if (i == axis)
		{
			return d->m_slicers[i]->GetOrientationMarkers_L();
		}
	}
	return "";
}

string MPR::GetOrientationMarkerRight(Axis axis)
{
	for (int i = 0; i<3; i++)
	{
		if (i == axis)
		{
			return d->m_slicers[i]->GetOrientationMarkers_R();
		}
	}
	return "";
}
string MPR::GetOrientationMarkerTop(Axis axis)
{
	for (int i = 0; i<3; i++)
	{
		if (i == axis)
		{
			return d->m_slicers[i]->GetOrientationMarkers_T();
		}
	}
	return "";
}
string MPR::GetOrientationMarkerBottom(Axis axis)
{
	for (int i = 0; i<3; i++)
	{
		if (i == axis)
		{
			return d->m_slicers[i]->GetOrientationMarkers_B();
		}
	}
	return "";
}

double MPR::GetPixelSpacing(int axis)
{
	switch (axis)
	{
		case 0:
		{
			return d->m_slicers[axis]->m_spacing[2];
		}
		case 1:
		{
			return d->m_slicers[axis]->m_spacing[1];
		}
		case 2:
		{
			return d->m_slicers[axis]->m_spacing[0];
		}
	}
}

long int MPR::GetPixelIntensity(Axis axis, int x_pos, int y_pos)
{
	for (int i = 0; i < 3; i++)
	{
		if (i == (int)axis)
		{
			return d->m_slicers[i]->GetPixelIntensity(x_pos, y_pos);
		}
	}
	return 0;
}