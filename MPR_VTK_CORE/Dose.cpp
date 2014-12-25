#include "Dose.h"

#include <algorithm>
#include <functional>

#include "rad_logger.h"
#include "rad_template_helper.h"
#include "rad_util.h"

#include "dcmtk\dcmjpeg\djdecode.h"  /* for dcmjpeg decoders */
#include "dcmtk\dcmjpeg\djencode.h"  /* for dcmjpeg encoders */
#include "dcmtk\dcmdata\dcrledrg.h"  /* for rle decoders */	
#include "dcmtk\dcmjpeg\djrplol.h"   /* for DJ_RPLossless */
#include "dcmtk\dcmjpeg\djrploss.h"  /* for DJ_RPLossy */

#include "vtkImageData.h"
#include "vtkUnsignedIntArray.h"
#include "vtkSignedCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkShortArray.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkPointData.h"

#include "vtkImageCast.h"
#include "vtkImageMapToColors.h"
#include "vtkLookupTable.h"
#include "vtkImageResample.h"
#include "DoseReslicer.h"


#include "DCMGeometry.h"
using namespace RTViewer;
#define __FILENAME__ "Dose.cpp"
#undef  __MODULENAME__
#define __MODULENAME__ "Dose"

#define ULTA 1
// RT Plan: Dose reference sequence
struct RTViewer::DoseReferenceSequence
{
	DoseReferenceSequence()
	{

	}
	U16DataType p_uniqueNumber;
	const char* p_UID;
	const char* p_structureType;
	const char* p_description;
	const char* p_type;
	float p_targetPrescriptionDose;
};

struct RTViewer::doseFrameData
{
	void* pixelData;
	//void* ORG_pixelData;
	int frameNumber;
	int rows;
	int cols;
	RadDataType dicomDataType; // will help in type casting.
	//RadDataType ORG_DicomDataType; // will help in type casting.
	doseFrameData(){}
};


// --------------------------------------------------------------
// ------------------- DOSE 
namespace RTViewer
{
	struct DoseData{
	public:

		//MPRTransform* transform;
		

		DoseReslicer* slicers[3];
		vtkSmartPointer<DCMGeometry> m_dcmGeometry;
		vtkSmartPointer<DCMGeometry> m_resampled_dcm_geometry;
		vtkImageData* m_doseCuboid;
		vtkImageData* m_resampledDoseCuboid;
	};
}
Dose::Dose( RTDcmtkDicomInterface* rtDoseDicom, Plan* rtPlan)
{
	d = new DoseData();
	d->m_dcmGeometry = vtkSmartPointer<DCMGeometry>::New();
	d->m_resampledDoseCuboid = NULL;
	d->m_resampled_dcm_geometry = NULL;
	DcmItem *ditem = NULL;
	const char * refPlanUID = NULL;
	this->frameRefUID = string(rtDoseDicom->Get_FRAME_OF_REFER_UID());
	this->sopUID = string(rtDoseDicom->Get_SOP_INSTANCE_UID());
	this->studyUID = string(rtDoseDicom->Get_STUDY_INSTANCE_UID());
	this->seriesUID = string(rtDoseDicom->Get_SERIES_INSTANCE_UID());
	
	this->height = rtDoseDicom->Get_ROW();
	this->width = rtDoseDicom->Get_COLOUMN();

	double image_orientation[6] = { 0, 0, 0, 0, 0, 0 };
	string imageOrientation = string(rtDoseDicom->Get_IMAGE_ORIENTATION());
	vector<string> _imageOrientation;
	tokenize(imageOrientation, _imageOrientation, "\\", true);
	for (int i = 0; i<_imageOrientation.size(); i++)
	{
		image_orientation[i] = convert_to_double(_imageOrientation.at(i).c_str());
	}

	double xorient[3] = { image_orientation[0], image_orientation[1], image_orientation [2]};
	double yorient[3] = { image_orientation[3], image_orientation[4], image_orientation [5]};


	double firstImagePosition[3] = { 0, 0, 0};
	double lastImagePosition[3] = { 0, 0, 0};
	string imagePosition = string(rtDoseDicom->Get_IMAGE_POSITION());
	vector<string> _imagePosition;
	tokenize(imagePosition, _imagePosition, "\\", true);
	for (int i = 0; i<_imagePosition.size(); i++)
	{
		firstImagePosition[i] = convert_to_double(_imagePosition.at(i).c_str());
		lastImagePosition[i] = convert_to_double(_imagePosition.at(i).c_str());
	}

	rtDoseDicom->getMinMaxPixelValue(this->minDosePixelValue, this->maxDosePixelValue);// MinMax pixel value

	this->doseGridScaling = convert_to_double(rtDoseDicom->Get_DOSEGRID_SCALING());

	OFString _gridFrameOffsetVector;
	vector<string> temp;
	if (rtDoseDicom->dataset->findAndGetOFStringArray(DCM_GridFrameOffsetVector, _gridFrameOffsetVector).bad())
	{
		isMultiframe = false;
		//		return;
	}
	else
	{
		isMultiframe = true;
		tokenize(_gridFrameOffsetVector.c_str(), temp, "\\", true);
		// Convert offset vector points from string to float and store permanently.
		for (int i = 0; i<temp.size(); i++)
		{
			this->doseGridOffsetVector.push_back(atof(temp.at(i).c_str()));
		}
	}

	if (isMultiframe)
		lastImagePosition[2] += doseGridOffsetVector.at(doseGridOffsetVector.size() - 1);
	else
		lastImagePosition[2] += 0;

	double spacing[3] = { 0, 0, 0 };
	const char* _pixelSpacing = rtDoseDicom->Get_PIXEL_SPACING();
	vector<string> temp1;
	tokenize(_pixelSpacing, temp1, "\\", true);
	spacing[0] = convert_to_double(temp1.at(0).c_str());
	spacing[1] = convert_to_double(temp1.at(1).c_str());
	if (isMultiframe)
	{
		spacing[2] = this->doseGridOffsetVector.at(1) - this->doseGridOffsetVector.at(0);// hardcoded.
	}
	else
	{
		//this->pixelSpacing[0] = 0;
		//this->pixelSpacing[1] = 0;
		spacing[2] = 0;
	}

	int numberOfFrames = convert_to_int(rtDoseDicom->Get_NO_OF_FRAMES());
	int dim[3] = { this->width, this->height, numberOfFrames };

	
	d->m_dcmGeometry->SetImageGeometry(firstImagePosition, spacing, dim, xorient, yorient);

	//Populate frame wise pixel data in map<int,void*> framePixelData;
	int noOfBytes = rtDoseDicom->Get_BITS_ALLOCATED() / 8;
	DJDecoderRegistration::registerCodecs();
	DcmRLEDecoderRegistration::registerCodecs();

	DicomImage * dicomImage = new DicomImage(&rtDoseDicom->file_format, rtDoseDicom->file_format.getDataset()->getOriginalXfer(),
		CIF_UsePartialAccessToPixelData, 0, 1 /* fcount */);

	DJDecoderRegistration::cleanup();
	DcmRLEDecoderRegistration::cleanup();

	

	do{
		const DiPixel* diPixel = dicomImage->getInterData();
		doseFrameData* dfData = new doseFrameData();
		dfData->frameNumber = dicomImage->getFirstFrame();
		dfData->cols = rtDoseDicom->Get_COLOUMN();
		dfData->rows = rtDoseDicom->Get_ROW();

		// create a vtkImage
		vtkImageData* image = vtkImageData::New();

		image->SetDimensions(dfData->cols, dfData->rows, 1);
		image->SetSpacing(spacing[0], spacing[1], 1);
		image->Initialize();

		vtkDataArray* scalars = 0;

		switch (diPixel->getRepresentation())
		{
			case EPR_Uint8:
				/*dfData->ORG_pixelData = calloc(diPixel->getCount(),sizeof(U8DataType));
				memcpy(dfData->ORG_pixelData,diPixel->getData(),diPixel->getCount()*sizeof(U8DataType));
				dfData->ORG_DicomDataType = TYPE_U8Data;*/

				scalars = vtkUnsignedCharArray::New();
				((vtkUnsignedCharArray*)(scalars))->SetArray((unsigned char*)diPixel->getData(), diPixel->getCount(), 1);
				break;

			case EPR_Sint8:

				/*dfData->ORG_pixelData = calloc(diPixel->getCount(),sizeof(S8DataType));
				memcpy(dfData->ORG_pixelData,diPixel->getData(),diPixel->getCount()*sizeof(S8DataType));
				dfData->ORG_DicomDataType = TYPE_S8Data;*/

				scalars = vtkSignedCharArray::New();
				((vtkSignedCharArray*)(scalars))->SetArray((signed char*)diPixel->getData(), diPixel->getCount(), 1);
				break;

			case EPR_Uint16:
				/*dfData->ORG_pixelData = calloc(diPixel->getCount(),sizeof(U16DataType));
				memcpy(dfData->ORG_pixelData,diPixel->getData(),diPixel->getCount()*sizeof(U16DataType));
				dfData->ORG_DicomDataType = TYPE_U16Data;*/

				scalars = vtkUnsignedShortArray::New();
				((vtkUnsignedShortArray*)(scalars))->SetArray((unsigned short*)diPixel->getData(), diPixel->getCount(), 1);
				break;

			case EPR_Sint16:

				/*dfData->ORG_pixelData = calloc(diPixel->getCount(),sizeof(S16DataType));
				memcpy(dfData->ORG_pixelData,diPixel->getData(),diPixel->getCount()*sizeof(S16DataType));
				dfData->ORG_DicomDataType = TYPE_S16Data;*/

				scalars = vtkShortArray::New();
				((vtkShortArray*)(scalars))->SetArray((short*)diPixel->getData(), diPixel->getCount(), 1);
				break;

			case EPR_Uint32:
				/*dfData->ORG_pixelData = calloc(diPixel->getCount(),sizeof(U32DataType));
				memcpy(dfData->ORG_pixelData,diPixel->getData(),diPixel->getCount()*sizeof(U32DataType));
				dfData->ORG_DicomDataType = TYPE_U32Data;*/

				scalars = vtkUnsignedIntArray::New();
				((vtkUnsignedIntArray*)(scalars))->SetArray((unsigned int*)diPixel->getData(), diPixel->getCount(), 1);

				break;

			case EPR_Sint32:
				/*dfData->ORG_pixelData = calloc(diPixel->getCount(),sizeof(S32DataType));
				memcpy(dfData->ORG_pixelData,diPixel->getData(),diPixel->getCount()*sizeof(S32DataType));
				dfData->ORG_DicomDataType = TYPE_S32Data;*/

				scalars = vtkIntArray::New();
				((vtkIntArray*)(scalars))->SetArray((int*)diPixel->getData(), diPixel->getCount(), 1);
				break;
			default:
				RAD_LOG_CRITICAL("DCMTK EP_Representation type:" << diPixel->getRepresentation() << " not supported");
		}

		scalars->SetNumberOfComponents(1);
		image->SetDimensions(dfData->cols, dfData->rows, 1);

		image->SetSpacing(spacing[0], spacing[1], 1.0);

		image->GetPointData()->SetScalars(scalars);
		image->GetPointData()->GetScalars()->SetName("DICOMImage");
		//image->Update();
		double bounds[6];
		image->GetBounds(bounds);

		// type cast all images to unsigned int32
		vtkImageCast* imageCast = vtkImageCast::New();
		imageCast->SetInputData(image);
		imageCast->SetOutputScalarTypeToFloat();
		imageCast->Update();

		dfData->pixelData = calloc(diPixel->getCount(), sizeof(U32DataType));
		dfData->dicomDataType = TYPE_U32Data;
		RAD_LOG_INFO("MEMCPY pixel data for frame:" << dicomImage->getFirstFrame())

			memcpy(dfData->pixelData, imageCast->GetOutput()->GetPointData()->GetScalars()->GetVoidPointer(0),
			imageCast->GetOutput()->GetPointData()->GetScalars()->GetSize()*sizeof(float));

		imageCast->Delete();
		scalars->Delete();
		image->Delete();

		this->allFrameDoseData.push_back(dfData);

	} while (dicomImage->processNextFrames());

	/// compute cuboid

	// get first dose frame data
	doseFrameData* dfData = allFrameDoseData.at(0);

	// create VTK Cuboid
	int dimension[3] = { dfData->cols, dfData->rows, allFrameDoseData.size() };
	long dicomSliceSize = dimension[0] * dimension[1];
	long dicomDataSize = dimension[0] * dimension[1] * dimension[2];
	long dicomDataIdx = dicomSliceSize;

	void* dicomData = rad_get_memory(dicomDataSize * sizeof(float));
#ifdef ULTA
	for (int i = allFrameDoseData.size() - 1; i >= 0; i--)
#else
	for (int i = 0;i< allFrameDoseData.size();i++)
#endif
	{
		float* dicomData2 = static_cast<float*>(dicomData);
		dicomData2 += dicomDataSize;
		memcpy(dicomData2 - dicomDataIdx, allFrameDoseData.at(i)->pixelData, dicomSliceSize*sizeof(float));
		dicomDataIdx += dicomSliceSize;
	}

	vtkDataArray* scalars = 0;
	scalars = vtkFloatArray::New();
	((vtkFloatArray*)(scalars))->SetArray((float*)dicomData, dicomDataSize, 1);
	scalars->SetNumberOfComponents(1);

	d->m_doseCuboid = vtkImageData::New();
	d->m_doseCuboid->SetDimensions(dimension);
	d->m_doseCuboid->SetSpacing(spacing);

	//double lip[3] = { 0, 0, 0 };
	//d->m_dcmGeometry->GetLastImagePosition(lip);
	//d->m_doseCuboid->SetOrigin(firstImagePosition);

	d->m_doseCuboid->GetPointData()->SetScalars(scalars);
	d->m_doseCuboid->GetPointData()->GetScalars()->SetName("Dose Cuboid");
	d->m_doseCuboid->Modified();



	rxDose = rtPlan->getTargetPrescribedDose();
	int dosemax = int(this->maxDosePixelValue * this->doseGridScaling * 10000 / rxDose);
	// ISODOSE LEVEL(s) READY
	this->isodoses.push_back(new isodose(dosemax, 120, 0, 0, "", false, 0, 0, "Built-In"));
	this->isodoses.push_back(new isodose(102, 170, 0, 0, "", false, 0, 0, "Built-In"));
	this->isodoses.push_back(new isodose(100, 238, 69, 0, "", false, 0, 0, "Built-In"));
	this->isodoses.push_back(new isodose(98, 255, 65, 0, "", false, 0, 0, "Built-In"));
	this->isodoses.push_back(new isodose(95, 255, 255, 0, "", false, 0, 0, "Built-In"));
	this->isodoses.push_back(new isodose(90, 0, 255, 0, "", false, 0, 0, "Built-In"));
	this->isodoses.push_back(new isodose(80, 0, 139, 0, "", false, 0, 0, "Built-In"));
	this->isodoses.push_back(new isodose(70, 0, 255, 255, "", false, 0, 0, "Built-In"));
	this->isodoses.push_back(new isodose(50, 0, 0, 255, "", false, 0, 0, "Built-In"));
	this->isodoses.push_back(new isodose(30, 0, 0, 128, "", false, 0, 0, "Built-In"));
	// Calculate cGy Value and append to names.
	for (int i = 0; i<this->isodoses.size(); i++)
	{
		isodose* iso = this->isodoses.at(i);
		iso->cGyValue = (int)(iso->level * rxDose / 100);
		iso->name.append(convert_to_string(iso->level));
		iso->name.append("% - ");
		iso->name.append(convert_to_string(iso->cGyValue));
		iso->name.append(" cGy");
		iso->pixelIntensity = iso->level * rxDose / (this->doseGridScaling * 10000);
	}

	// load DV data
	DcmItem *dvhItem = NULL;
	signed long dvhCount = 0;
	while (rtDoseDicom->dataset->findAndGetSequenceItem(DCM_DVHSequence, dvhItem, dvhCount++).good())
	{
		DVH* dh = new DVH();

		DcmItem *dvhRefROISequenceItem = NULL;

		// ROI Number
		const char * RoiNumber = NULL;
		if (dvhItem->findAndGetSequenceItem(DCM_DVHReferencedROISequence, dvhRefROISequenceItem, 0).good())
		{
			OFCondition status = dvhItem->findAndGetString(DCM_ReferencedROINumber, RoiNumber, true);
			dh->setROINumber(convert_to_uint(RoiNumber));
		}

		// DVH Data
		string fNm = "D:\\DVH_DICOM_";
		fNm.append(convert_to_string(dvhCount));
		fNm.append(".csv");
		//ofstream Morison_File(fNm.c_str());

		const char * dvhdata = NULL;
		dvhItem->findAndGetString(DCM_DVHData, dvhdata);

		vector<string> dvhdataListTemp;
		tokenize(dvhdata, dvhdataListTemp, "\\");
		for (int i = 0; i < dvhdataListTemp.size(); i++)
		{
			i++;
			string volume = dvhdataListTemp.at(i);
			if (0 > int(atof(volume.c_str())))
				volume = "0";
			dh->dvhDataList.push_back(convert_to_double(volume.c_str()));
	//		Morison_File << volume.c_str() << endl;
		}
		//Morison_File.close();

		//convert volume information in dvhDataList from abosulte to percentage.
		dh->setMaxDVHValue(*std::max_element(dh->dvhDataList.begin(), dh->dvhDataList.end()));

		/*std::transform(dh->dvhDataList.begin(), dh->dvhDataList.end(),
			dh->dvhDataList.begin(), std::bind2nd(std::divides<double>(), dh->getMaxDVHValue()));

		std::transform(dh->dvhDataList.begin(), dh->dvhDataList.end(),
			dh->dvhDataList.begin(), std::bind2nd(std::multiplies<double>(), 100));*/


		// DVH Mini Dose
		const char * dvhMinDose = NULL;
		if (dvhItem->findAndGetString(DCM_DVHMinimumDose, dvhMinDose).good())
			dh->setDVHMiniDose(convert_to_double(dvhMinDose));

		//DVH Max Dose
		const char * dvhMaxDose = NULL;
		if (dvhItem->findAndGetString(DCM_DVHMaximumDose, dvhMaxDose).good())
			dh->setDVHMaxDose(convert_to_double(dvhMaxDose));

		//DVH Mean Dose
		const char * dvhMeanDose = NULL;
		if (dvhItem->findAndGetString(DCM_DVHMeanDose, dvhMeanDose).good())
			dh->setDVHMeanDose(convert_to_double(dvhMeanDose));

		// Structureset ref SOPUID
		DcmItem *dvhRefStructureSet = NULL;
		const char * structSetRefSOPUID = NULL;
		if (rtDoseDicom->dataset->findAndGetSequenceItem(DCM_ReferencedStructureSetSequence, dvhRefStructureSet).good())
		{
			if (dvhRefStructureSet->findAndGetString(DCM_ReferencedSOPInstanceUID, structSetRefSOPUID, true).good())
			dh->setStrucSetRefSopUID(string(structSetRefSOPUID));
		}
		this->dh.push_back(dh);
	}
}


Dose::~Dose()
{
}

void Dose::InitMPRPipeline()
{
	//d->transform = MPRTransform::New();

	for (int i = 0; i < 3; i++)
	{
		DoseReslicer* slicer = new DoseReslicer((Axis)i);
		slicer->SetInput(d->m_doseCuboid);
		//slicer->InitSlicer(NULL, d->transform);

		d->slicers[i] = slicer;
	}

	//d->transform->Identity();
	//d->transform->Translate(d->m_doseCuboid->GetCenter()); // translate to center
	//d->transform->ResetRotations();

}

void Dose::Scroll3(Axis axis, double pos)
{
	/*d->slicers[(int)axis]->GetOuputImage()->GetMatrix()->SetElement(0, 3, 0);
	d->slicers[(int)axis]->GetTransform()->GetMatrix()->SetElement(1, 3, 0);
	d->slicers[(int)axis]->GetTransform()->GetMatrix()->SetElement(2, 3, pos);*/
}

void Dose::Scroll(Axis axis, int delta)
{

	/*double spacing[3];
	d->m_dcmGeometry->GetSpacing(spacing);

	double pos[] = { 0, 0, 0 };
	d->slicers[(int)axis]->GetTransform()->TransformPoint(pos, pos);
	RAD_LOG_CRITICAL("Translation delta:" << delta);
	RAD_LOG_CRITICAL("Initial Position x:" << pos[0] << " y:" << pos[1] << " z:" << pos[2]);
	double _spacing = spacing[2];

	switch (axis)
	{
		case RTViewer::AxialAxis:

			break;
		case RTViewer::CoronalAxis:
			_spacing = spacing[1];
			break;
		case RTViewer::SagittalAxis:
			_spacing = spacing[0];
			break;
		default:
			break;
	}
	double t[3] = { 0, 0, -delta*_spacing };
	d->slicers[(int)axis]->GetTransform()->TransformPoint(t, t);
	RAD_LOG_CRITICAL("Translation vector x:" << t[0] << " y:" << t[1] << " z:" << t[2]);
	t[0] -= pos[0]; t[1] -= pos[1]; t[2] -= pos[2];
	RAD_LOG_CRITICAL("After translation x:" << t[0] << " y:" << t[1] << " z:" << t[2]);
	double bounds[] = { 0, 0, 0, 0, 0, 0 };
	d->m_doseCuboid->GetBounds(bounds);

	for (int i = 0; i<3; i++)
	{
		double v = pos[i] + t[i];
		if (v < bounds[i * 2] || v > bounds[i * 2 + 1])
		{
			RAD_LOG_CRITICAL("Going out of bounds. Returning")
				return;
		}
	}

	d->transform->Translate(t);*/

}

void Dose::Scroll2(Axis axis, float dx, float dy)
{
	// Find out the current postion
	//double pos[] = { 0, 0, 0 };
	//d->slicers[(int)axis]->GetTransform()->TransformPoint(pos, pos);

	//// Translate the current slice matrix
	//double t[] = { dx, dy, 0 };
	//d->slicers[(int)axis]->GetTransform()->Translate(t[0], t[1], t[2]);

	//// Find out the translated position
	//double tpos[] = { 0, 0, 0 };
	//d->slicers[(int)axis]->GetTransform()->TransformPoint(tpos, tpos);

	//// Translate the slice matrix back to its previous position
	//d->slicers[(int)axis]->GetTransform()->Translate(-t[0], -t[1], -t[2]);

	//// Evaluate the actual translation
	//for (int i = 0; i < 3; i++)
	//	t[i] = tpos[i] - pos[i];

	//// Ensure that tpos is not outside bounds
	//double bounds[] = { 0, 0, 0, 0, 0, 0 };
	//d->m_doseCuboid->GetBounds(bounds);
	//for (int i = 0; i < 3; i++)
	//{
	//	double& v = tpos[i];
	//	if (v < bounds[i * 2] || v > bounds[i * 2 + 1])
	//		return;
	//}

	//// Apply the translation on the cursor and update.
	//d->transform->Translate(t);


}


vtkSmartPointer<vtkImageData> Dose::ComputeColorWash2(int axis, int idx, double& translateX, double& translateY)
{
	double spacing[3];
	d->m_dcmGeometry->GetSpacing(spacing);
	double _spacing = spacing[2];
	switch (axis)
	{
		case RTViewer::AxialAxis:

			break;
		case RTViewer::CoronalAxis:
			_spacing = spacing[1];
			break;
		case RTViewer::SagittalAxis:
			_spacing = spacing[0];
			break;
		default:
			break;
	}
	double pos = idx* _spacing;

	return this->ComputeColorWash(axis, pos, translateX, translateY);

}
vtkSmartPointer<vtkImageData> Dose::ComputeColorWash(int axis, double pos, double& translateX, double& translateY)
{
	for (int i = 0; i < 3; i++)
	{
		if (i == axis)
		{
			
			//Scroll((Axis)axis, slicerIdx);
			// find delta from old position
			//int current_idx = d->slicers[(int)axis]->GetSlicerPositionAsIndex();
			//int delta = slicerIdx-current_idx;


			//// translate
			/*double spacing[3];
			d->m_dcmGeometry->GetSpacing(spacing);*/

			//double pos[] = { 0, 0, 0 };
			//d->slicers[(int)axis]->GetTransform()->TransformPoint(pos, pos);
			//
			//RAD_LOG_CRITICAL("<DOSE> Translation delta:" << delta);
			//RAD_LOG_CRITICAL("<DOSE> Initial Position x:" << pos[0] << " y:" << pos[1] << " z:" << pos[2]);
			/*double _spacing = spacing[2];

			switch (axis)
			{
				case RTViewer::AxialAxis:

					break;
				case RTViewer::CoronalAxis:
					_spacing = spacing[1];
					break;
				case RTViewer::SagittalAxis:
					_spacing = spacing[0];
					break;
				default:
					break;
			}
			pos = */
			//double t[3] = { 0, 0, delta*_spacing };
			//d->slicers[(int)axis]->GetTransform()->TransformPoint(t, t);
			//RAD_LOG_CRITICAL("<DOSE> Translation vector x:" << t[0] << " y:" << t[1] << " z:" << t[2]);
			//t[0] -= pos[0]; t[1] -= pos[1]; t[2] -= pos[2];
			//RAD_LOG_CRITICAL("<DOSE> After translation x:" << t[0] << " y:" << t[1] << " z:" << t[2]);
			//double bounds[] = { 0, 0, 0, 0, 0, 0 };
			//d->m_doseCuboid->GetBounds(bounds);

			//for (int i = 0; i<3; i++)
			//{
			//	double v = pos[i] + t[i];
			//	if (v < bounds[i * 2] || v > bounds[i * 2 + 1])
			//	{
			//		RAD_LOG_CRITICAL("<DOSE> Going out of bounds. Returning")
			//			return NULL;
			//	}
			//}

			//d->transform->Translate(t);

			// get raw output image
			vtkSmartPointer<vtkImageData> image = d->slicers[(int)axis]->GetOuputImage(pos);

			// get lookup table.
			//vtkLookupTable *RT_LUT_Table = vtkLookupTable::New();

			//RT_LUT_Table->SetRange(0, 969000); // image intensity range
			//RT_LUT_Table->SetNumberOfColors(7);
			//double opacity = 0.3;
			//RT_LUT_Table->SetTableValue(0, 0, 0, 1, opacity);
			//RT_LUT_Table->SetTableValue(1, 0, 1.0, 0, opacity);
			//RT_LUT_Table->SetTableValue(2, 0.6, 1.0, 0.0, opacity);
			//RT_LUT_Table->SetTableValue(3, 1.0, 1.0, 0.0, 0.7);
			//RT_LUT_Table->SetTableValue(4, 1.0, 0.8, 0.0, opacity);
			//RT_LUT_Table->SetTableValue(5, 1.0, 0.4, 0.0, opacity);
			//RT_LUT_Table->SetTableValue(6, 1.0, 0.0, 0.0, 1);

			vtkLookupTable* lut = JetColorMap::GetColorMap();
			lut->SetAlpha(0.7);
			//TODO: set correct values.
			//969000
			lut->SetTableRange(0, this->isodoses.at(0)->pixelIntensity); // 30% to 100%
			lut->Build();

			//// Map the image through the lookup table
			vtkSmartPointer<vtkImageMapToColors> RT_Color = vtkSmartPointer<vtkImageMapToColors>::New();
			RT_Color->SetInputData(image);
			RT_Color->SetLookupTable(lut);
			RT_Color->Update();
			image = RT_Color->GetOutput();
			return image;
		}
	}
	return NULL;
}

DCMGeometry* Dose::GetDCMGeometry()
{
	return d->m_dcmGeometry;
}

int Dose::GetCurrentImageIndex(Axis axis)
{
	int idx = 0;
	/*for (int i = 0; i<3; i++)
	{
		if (i == axis)
		{
			idx = d->slicers[i]->GetSlicerPositionAsIndex();
		}
	}*/
	return idx;
}

double Dose::GetCurrentImagePosition(Axis axis)
{
	double pos = 0;
	//for (int i = 0; i<3; i++)
	//{
	//	if (i == axis)
	//	{
	//		pos = d->m_dcmGeometry->ComputePosition(GetCurrentImageIndex(axis), axis);// d->m_slicers[i]->GetSlicerPosition();
	//	}
	//}
	return pos;
}

#include "vtkContourFilter.h"
#include "vtkStripper.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"

void Dose::ComputeROI(int axis, int idx, double targetSpacing[3], double DOSELIP[3], vector<vector<vector<dosePlotPoint>> >& outPoints)
{
	//double targetSpacing[2] = { 1.3672, 1.3672 };
	double spacing[3];
	d->m_dcmGeometry->GetSpacing(spacing);

	double doseFIP[3] = { 0, 0, 0 };
	d->m_dcmGeometry->GetLastImagePosition(doseFIP);

	d->m_dcmGeometry->GetSpacing(spacing);
	double _spacing = spacing[2];
	switch (axis)
	{
		case RTViewer::AxialAxis:

			break;
		case RTViewer::CoronalAxis:
			_spacing = spacing[1];
			break;
		case RTViewer::SagittalAxis:
			_spacing = spacing[0];
			break;
		default:
			break;
	}
	double pos = idx* _spacing;
	
	vtkSmartPointer<vtkImageData> image = d->slicers[axis]->GetOuputImage(pos);
	double origin[3] = { 0, 0, 0 };
	image->GetOrigin(origin);

	for (int i = isodoses.size() - 1; i >= 0; i--)
	{
		
		vtkContourFilter* contourFilter = vtkContourFilter::New();
		contourFilter->SetInputData(d->slicers[axis]->GetOuputImage(pos));
		contourFilter->GenerateValues(1, isodoses.at(i)->pixelIntensity, isodoses.at(i)->pixelIntensity);

		contourFilter->Update();

		vector<vector<dosePlotPoint>> doseCurvePointsForAllImages;
		vtkStripper* stripper = vtkStripper::New();
		stripper->SetInputConnection(contourFilter->GetOutputPort());
		stripper->Update();

		// get polydata
		vtkPolyData* polyData = stripper->GetOutput();
		vtkCellArray* verts = polyData->GetLines();
		vtkPoints* pPoints = polyData->GetPoints();
		vtkIdType nPts = 0;
		vtkIdType *ptIds = verts->GetPointer();

		doseCurvePointsForAllImages.resize(verts->GetNumberOfCells());
		int count = 0;
		
		

		for (verts->InitTraversal(); verts->GetNextCell(nPts, ptIds);)
		{
			for (int j = 0; j < nPts; j++)
			{
				double v[3];
				pPoints->GetPoint(ptIds[j], v);
				// Note: Substracting from origin; will translate point to (0,0) origin.
				v[0] -= origin[0];
				v[1] -= origin[1];
				v[2] -= origin[2];

				// transform point for target image spacing
				v[0] = v[0] / targetSpacing[0]; //x
				v[1] = v[1] / targetSpacing[1]; //y

				if (axis == 0)
				{
					//v[0] += (DOSELIP[0] - CTFIP[0]) / targetSpacing[0];
					//v[1] += (DOSELIP[1] - CTFIP[1]) / targetSpacing[1];

				}
				dosePlotPoint p;

				p.x = v[0] * targetSpacing[0];		
				p.y = v[1] * targetSpacing[1];
				p.z = v[2];
				p.level = isodoses.at(i)->level;
				doseCurvePointsForAllImages.at(count).push_back(p);
			}
			count++;
		}
		outPoints.push_back(doseCurvePointsForAllImages);
	}

}
//void Dose::ComputeISODOSE(int axis, vtkImageData* imageData, double targetSpacing[2], double CTFirstImagePosition[3], vector<vector<vector<dosePlotPoint>> >& outPoints)
//{
//	for (int i = isodoses.size() - 1; i >= 0; i--)
//	{
//		//if (isodoses.at(i)->checked == true)
//		{
//			vtkContourFilter* contourFilter = vtkContourFilter::New();
//			contourFilter->SetInputData(imageData);
//			contourFilter->GenerateValues(1, isodoses.at(i)->pixelIntensity, isodoses.at(i)->pixelIntensity);
//			contourFilter->Update();
//			vector<vector<dosePlotPoint>> doseCurvePointsForAllImages;
//			vtkStripper* stripper = vtkStripper::New();
//			stripper->SetInputConnection(contourFilter->GetOutputPort());
//			stripper->Update();
//
//			// get polydata
//			vtkPolyData* polyData = stripper->GetOutput();
//			vtkCellArray* verts = polyData->GetLines();
//			vtkPoints* pPoints = polyData->GetPoints();
//			vtkIdType nPts = 0;
//			vtkIdType *ptIds = verts->GetPointer();
//
//			doseCurvePointsForAllImages.resize(verts->GetNumberOfCells());
//			int count = 0;
//
//			double origin[3];
//			imageData->GetOrigin(origin);
//
//			for (verts->InitTraversal(); verts->GetNextCell(nPts, ptIds);)
//			{
//				for (int j = 0; j < nPts; j++)
//				{
//					double v[3];
//					pPoints->GetPoint(ptIds[j], v);
//					// Note: Substracting from origin; will translate point to (0,0) origin.
//					v[0] -= origin[0];
//					v[1] -= origin[1];
//					v[2] -= origin[2];
//
//					// transform point for target image spacing
//					v[0] = v[0] / targetSpacing[0]; //x
//					v[1] = v[1] / targetSpacing[1]; //y
//
//					//axial
//					if (axis == (int)AxialAxis)
//					{
//						// translate to new origin
//						v[0] += (this->firstImagePatientPosition[0] - CTFirstImagePosition[0]) / targetSpacing[0];
//						v[1] += (this->firstImagePatientPosition[1] - CTFirstImagePosition[1]) / targetSpacing[1];
//
//						/*v[0] +=(-185.16 - (-329.6))/ targetSpacing[0];
//						v[1] +=(-180.0 - (-319.0)) / targetSpacing[1];*/
//					}
//
//					if (axis == (int)CoronalAxis)
//						// coronal XZ-Plane
//					{
//
//						// translate to new origin
//						v[0] += (this->firstImagePatientPosition[0] - CTFirstImagePosition[0]) / targetSpacing[0];
//						v[1] += (this->firstImagePatientPosition[2] - CTFirstImagePosition[2]) / targetSpacing[1];
//
//						/*v[0] +=(-185.16 - (-329.6))/targetSpacing[0];
//						v[1] +=(-51.3 - (-51.3))/targetSpacing[1];*/
//					}
//
//					if (axis == (int)SagittalAxis)
//						// Sagittal YZ-Plane
//					{
//						// translate to new origin
//						v[0] += (this->firstImagePatientPosition[1] - CTFirstImagePosition[1]) / targetSpacing[0];
//						v[1] += (this->firstImagePatientPosition[2] - CTFirstImagePosition[2]) / targetSpacing[1];
//
//						/*v[0] +=(-180.0 - (-319.0)) / targetSpacing[0];
//						v[1] +=(-51.3 - (-51.3))/targetSpacing[1];*/
//					}
//					dosePlotPoint p;
//
//					p.x = v[0];					p.y = v[1];					p.z = v[2];
//					p.level = isodoses.at(i)->level;
//					doseCurvePointsForAllImages.at(count).push_back(p);
//
//				}
//				count++;
//			}
//			//MyFile.close();
//			outPoints.push_back(doseCurvePointsForAllImages);
//		}
//	}
//}

vtkImageData* Dose::GetVolume()
{
	return d->m_doseCuboid;
}

vtkImageData* Dose::GetResampledVolume()
{
	return d->m_resampledDoseCuboid;
}

DCMGeometry* Dose::GetResampledDCMGeometry()
{
	return d->m_resampled_dcm_geometry;
}
#include "vtkImageResample.h"
#include "vtkImageChangeInformation.h"

vtkImageData* Dose::Resample(double sx, double sy, double sz)
{

	double spacing[3] = { 0, 0, 0 };
	d->m_dcmGeometry->GetSpacing(spacing);
	double resampled_spacing[3] = { spacing[0] / sx, spacing[1] / sy, spacing[2] / sz };
/*
	RAD_LOG_CRITICAL("Unsampled data: Begin ....................");
	d->m_doseCuboid->Print(cerr);
	RAD_LOG_CRITICAL("Unsampled data: End   ....................");
	vtkImageResample* resampler1 = vtkImageResample::New();

	resampler1->SetInputData(d->m_doseCuboid);
	resampler1->SetDimensionality(3);
	resampler1->SetAxisOutputSpacing(0, resampled_spacing[0]);
	resampler1->SetAxisOutputSpacing(1, resampled_spacing[1]);
	resampler1->SetAxisOutputSpacing(2, resampled_spacing[2]);

	resampler1->SetInterpolationModeToCubic();
	resampler1->Update();

	vtkImageData* resampled_image = resampler1->GetOutput();
	RAD_LOG_CRITICAL("Resampled data: Begin ....................");
	resampled_image->Print(cerr);
	RAD_LOG_CRITICAL("Resampled data: End   ....................");
*/

	//if (d->m_resampledDoseCuboid != NULL)
	//{
	//	d->m_resampledDoseCuboid->Delete();
	//	d->m_resampledDoseCuboid = NULL;

	//	d->m_resampled_dcm_geometry->Delete();
	//}


	double origin[3] = { 0, 0, 0 };
	d->m_dcmGeometry->GetOrigin(origin);
	double resampled_origin[3] = { origin[0] / sx, origin[1] / sy, origin[2] / sz };

	int dim[3] = { 0, 0, 0 };
	d->m_dcmGeometry->GetDimensions(dim);

	double xOrient[3] = { 0, 0, 0 };
	double yOrient[3] = { 0, 0, 0 };
	double zOrient[3] = { 0, 0, 0 };

	d->m_dcmGeometry->GetOrientation(xOrient, yOrient, zOrient);

	d->m_resampled_dcm_geometry = DCMGeometry::New();
	d->m_resampled_dcm_geometry->SetImageGeometry(resampled_origin, resampled_spacing, dim, xOrient, yOrient);


	vtkImageResample* resampler = vtkImageResample::New();
	
	resampler->SetInputData(d->m_doseCuboid);
	resampler->SetDimensionality(3);
	resampler->SetAxisOutputSpacing(0, fabs(resampled_spacing[0]));
	resampler->SetAxisOutputSpacing(1, fabs(resampled_spacing[1]));
	resampler->SetAxisOutputSpacing(2, fabs(resampled_spacing[2]));
	
	resampler->SetInterpolationModeToCubic();
	resampler->Update();

	resampler->Print(cerr);
	vtkImageChangeInformation* changeInfo =	vtkImageChangeInformation::New();
	changeInfo->SetInputConnection(resampler->GetOutputPort());
	changeInfo->SetOutputSpacing(resampled_spacing);
	changeInfo->Update();

	d->m_resampledDoseCuboid = changeInfo->GetOutput();
	RAD_LOG_CRITICAL("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
	d->m_resampledDoseCuboid->Print(cerr);
	RAD_LOG_CRITICAL(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
	return d->m_resampledDoseCuboid;
}

#include ".\RT_StructureSet\vtkRTSSMPR.h"
#include ".\RT_StructureSet\RTSS.h"
#include "DVHComputation.h"
void Dose::ComputeDVH(StructureSetROISequence* roiSequence)
{
	DVHComputation* dvhComp = new DVHComputation();
	dvhComp->Init(roiSequence->getReslicer(), this);
	dvhComp->ComputeHistogram(0.01);

	DVH* dh = new DVH();
	dh->setROINumber(roiSequence->getNumber());

	dh->dvhDataList = dvhComp->GetVolumeBins();
	dh->setMaxDVHValue(*std::max_element(dh->dvhDataList.begin(), dh->dvhDataList.end()));

	dh->setDVHMaxDose(dvhComp->GetMaxDoseValue(DoseUnit::Percent));
	dh->setDVHMiniDose(dvhComp->GetMinDoseValue(DoseUnit::Percent));
	dh->setDVHMeanDose(dvhComp->GetMeanDoseValue(DoseUnit::Percent));
	//dh->dvhDataList.push_back(convert_to_double(volume.c_str()));

	this->computed_dvh.push_back(dh);
}
/// ---------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------
// ------------------- Plan 
Plan::Plan(RTDcmtkDicomInterface* rtPlanDicom)
{
	this->rxDose = 0.0;
	// Plan dose reference sequence
	DcmItem* doseRefSeqItem = NULL;
	int doseRefSeqItemCount = 0;
	this->studyUID = string(rtPlanDicom->Get_STUDY_INSTANCE_UID());
	if (rtPlanDicom->Get_FRAME_OF_REFER_UID() != NULL)
		this->frameRefUID = string(rtPlanDicom->Get_FRAME_OF_REFER_UID());
	this->sopUID = string(rtPlanDicom->Get_SOP_INSTANCE_UID());
	this->rtPlanLabel = string(rtPlanDicom->Get_RT_PLAN_LABEL());


	while (rtPlanDicom->dataset->findAndGetSequenceItem(DCM_DoseReferenceSequence, doseRefSeqItem, doseRefSeqItemCount++).good())
	{
		RAD_LOG_INFO(">>RT PLAN DOSE REF SEQUENCE:#" << doseRefSeqItemCount);
		const char* structType = NULL;
		doseRefSeqItem->findAndGetString(DCM_DoseReferenceStructureType, structType);
		bool _isSupported = false;
		if (strcmp(structType, "SITE") == 0 || strcmp(structType, "VOLUME") == 0)
		{
			_isSupported = true;
		}
		if (_isSupported)
		{

			DoseReferenceSequence* _rtplanDoseRef = new DoseReferenceSequence();
			_rtplanDoseRef->p_structureType = structType;

			const char* num = NULL;
			doseRefSeqItem->findAndGetString(DCM_DoseReferenceNumber, num);
			if (num != NULL)
			{
				_rtplanDoseRef->p_uniqueNumber = atoi(num);
			}
			doseRefSeqItem->findAndGetString(DCM_DoseReferenceDescription, _rtplanDoseRef->p_description);
			num = NULL;
			doseRefSeqItem->findAndGetString(DCM_TargetPrescriptionDose, num);
			if (num != NULL)
			{
				int rxdose = convert_to_double(num) * 100;
				if (rxdose > this->rxDose)
					this->rxDose = rxdose;
				_rtplanDoseRef->p_targetPrescriptionDose = atof(num);
			}
			doseRefSeqItem->findAndGetString(DCM_DoseReferenceType, _rtplanDoseRef->p_type);
			doseRefSeqItem->findAndGetString(DCM_DoseReferenceUID, _rtplanDoseRef->p_UID);
			//_rtplanDoseRef->p_uniqueNumber=
			this->DoseReferenceSequences.push_back(_rtplanDoseRef);


		}
		else
			RAD_LOG_CRITICAL("Dose structure type:" << structType << " is not supported");
	}

	DcmItem* fractionGroupSequence = NULL;
	int fractionGroupSequenceItemCount = 0;
	// RXDose not found in dose refernece sequence. Let's calculate dose from FractionSequence
	if (this->rxDose == 0.0 && rtPlanDicom->dataset->findAndGetSequenceItem(DCM_FractionGroupSequence, fractionGroupSequence, fractionGroupSequenceItemCount++).good())
	{
		// find the number of fractions planned.
		const char* numberOfFractionsPlanned = NULL;
		if (fractionGroupSequence->findAndGetString(DCM_NumberOfFractionsPlanned, numberOfFractionsPlanned).good())
		{
			// get referenced beam sequences.
			DcmItem* referencedBeamSequence = NULL;
			int referencedBeamSequenceItemCount = 0;
			// iterate over refernced beam sequences.
			while (fractionGroupSequence->findAndGetSequenceItem(DCM_ReferencedBeamSequence, referencedBeamSequence, referencedBeamSequenceItemCount++).good())
			{
				//find dose beam value
				const char* beamDose = NULL;
				if (referencedBeamSequence->findAndGetString(DCM_BeamDose, beamDose).good())
				{
					this->rxDose += convert_to_double(beamDose)* convert_to_int(numberOfFractionsPlanned) * 100; // formula taken from DICOMPyler code base.
				}
			}
		}
		else
		{
			RAD_LOG_CRITICAL("Tag: DCM_NumberOfFractionsPlanned not present in Fraction Group Sequence.");
			RAD_LOG_CRITICAL("RxDose is set to 0.00cGy");
		}

	}
}


Plan::~Plan()
{
}

float Plan::getTargetPrescribedDose()
{
	//float rxDose = 0.0F;
	//for (int i = 0; i<this->DoseReferenceSequences.size(); i++)
	//{
	//	if (this->DoseReferenceSequences.at(i)->p_targetPrescriptionDose > rxDose)
	//	{
	//		rxDose = this->DoseReferenceSequences.at(i)->p_targetPrescriptionDose * 100;
	//	}
	//}
	return rxDose;
}




JetColorMap* JetColorMap::_colorMapHandle = NULL;

JetColorMap::JetColorMap(void)
{
	RGBColor* jetArray[] = {
		new RGBColor(0, 0, 5.200000e-01),
		new RGBColor(0, 0, 5.400000e-01),
		new RGBColor(0, 0, 5.600000e-01),
		new RGBColor(0, 0, 5.800000e-01),
		new RGBColor(0, 0, 6.000000e-01),
		new RGBColor(0, 0, 6.200000e-01),
		new RGBColor(0, 0, 6.400000e-01),
		new RGBColor(0, 0, 6.600000e-01),
		new RGBColor(0, 0, 6.800000e-01),
		new RGBColor(0, 0, 7.000000e-01),
		new RGBColor(0, 0, 7.200000e-01),
		new RGBColor(0, 0, 7.400000e-01),
		new RGBColor(0, 0, 7.600000e-01),
		new RGBColor(0, 0, 7.800000e-01),
		new RGBColor(0, 0, 8.000000e-01),
		new RGBColor(0, 0, 8.200000e-01),
		new RGBColor(0, 0, 8.400000e-01),
		new RGBColor(0, 0, 8.600000e-01),
		new RGBColor(0, 0, 8.800000e-01),
		new RGBColor(0, 0, 9.000000e-01),
		new RGBColor(0, 0, 9.200000e-01),
		new RGBColor(0, 0, 9.400000e-01),
		new RGBColor(0, 0, 9.600000e-01),
		new RGBColor(0, 0, 9.800000e-01),
		new RGBColor(0, 0, 1),
		new RGBColor(0, 2.000000e-02, 1),
		new RGBColor(0, 4.000000e-02, 1),
		new RGBColor(0, 6.000000e-02, 1),
		new RGBColor(0, 8.000000e-02, 1),
		new RGBColor(0, 1.000000e-01, 1),
		new RGBColor(0, 1.200000e-01, 1),
		new RGBColor(0, 1.400000e-01, 1),
		new RGBColor(0, 1.600000e-01, 1),
		new RGBColor(0, 1.800000e-01, 1),
		new RGBColor(0, 2.000000e-01, 1),
		new RGBColor(0, 2.200000e-01, 1),
		new RGBColor(0, 2.400000e-01, 1),
		new RGBColor(0, 2.600000e-01, 1),
		new RGBColor(0, 2.800000e-01, 1),
		new RGBColor(0, 3.000000e-01, 1),
		new RGBColor(0, 3.200000e-01, 1),
		new RGBColor(0, 3.400000e-01, 1),
		new RGBColor(0, 3.600000e-01, 1),
		new RGBColor(0, 3.800000e-01, 1),
		new RGBColor(0, 4.000000e-01, 1),
		new RGBColor(0, 4.200000e-01, 1),
		new RGBColor(0, 4.400000e-01, 1),
		new RGBColor(0, 4.600000e-01, 1),
		new RGBColor(0, 4.800000e-01, 1),
		new RGBColor(0, 5.000000e-01, 1),
		new RGBColor(0, 5.200000e-01, 1),
		new RGBColor(0, 5.400000e-01, 1),
		new RGBColor(0, 5.600000e-01, 1),
		new RGBColor(0, 5.800000e-01, 1),
		new RGBColor(0, 6.000000e-01, 1),
		new RGBColor(0, 6.200000e-01, 1),
		new RGBColor(0, 6.400000e-01, 1),
		new RGBColor(0, 6.600000e-01, 1),
		new RGBColor(0, 6.800000e-01, 1),
		new RGBColor(0, 7.000000e-01, 1),
		new RGBColor(0, 7.200000e-01, 1),
		new RGBColor(0, 7.400000e-01, 1),
		new RGBColor(0, 7.600000e-01, 1),
		new RGBColor(0, 7.800000e-01, 1),
		new RGBColor(0, 8.000000e-01, 1),
		new RGBColor(0, 8.200000e-01, 1),
		new RGBColor(0, 8.400000e-01, 1),
		new RGBColor(0, 8.600000e-01, 1),
		new RGBColor(0, 8.800000e-01, 1),
		new RGBColor(0, 9.000000e-01, 1),
		new RGBColor(0, 9.200000e-01, 1),
		new RGBColor(0, 9.400000e-01, 1),
		new RGBColor(0, 9.600000e-01, 1),
		new RGBColor(0, 9.800000e-01, 1),
		new RGBColor(0, 1, 1),
		new RGBColor(2.000000e-02, 1, 9.800000e-01),
		new RGBColor(4.000000e-02, 1, 9.600000e-01),
		new RGBColor(6.000000e-02, 1, 9.400000e-01),
		new RGBColor(8.000000e-02, 1, 9.200000e-01),
		new RGBColor(1.000000e-01, 1, 9.000000e-01),
		new RGBColor(1.200000e-01, 1, 8.800000e-01),
		new RGBColor(1.400000e-01, 1, 8.600000e-01),
		new RGBColor(1.600000e-01, 1, 8.400000e-01),
		new RGBColor(1.800000e-01, 1, 8.200000e-01),
		new RGBColor(2.000000e-01, 1, 8.000000e-01),
		new RGBColor(2.200000e-01, 1, 7.800000e-01),
		new RGBColor(2.400000e-01, 1, 7.600000e-01),
		new RGBColor(2.600000e-01, 1, 7.400000e-01),
		new RGBColor(2.800000e-01, 1, 7.200000e-01),
		new RGBColor(3.000000e-01, 1, 7.000000e-01),
		new RGBColor(3.200000e-01, 1, 6.800000e-01),
		new RGBColor(3.400000e-01, 1, 6.600000e-01),
		new RGBColor(3.600000e-01, 1, 6.400000e-01),
		new RGBColor(3.800000e-01, 1, 6.200000e-01),
		new RGBColor(4.000000e-01, 1, 6.000000e-01),
		new RGBColor(4.200000e-01, 1, 5.800000e-01),
		new RGBColor(4.400000e-01, 1, 5.600000e-01),
		new RGBColor(4.600000e-01, 1, 5.400000e-01),
		new RGBColor(4.800000e-01, 1, 5.200000e-01),
		new RGBColor(5.000000e-01, 1, 5.000000e-01),
		new RGBColor(5.200000e-01, 1, 4.800000e-01),
		new RGBColor(5.400000e-01, 1, 4.600000e-01),
		new RGBColor(5.600000e-01, 1, 4.400000e-01),
		new RGBColor(5.800000e-01, 1, 4.200000e-01),
		new RGBColor(6.000000e-01, 1, 4.000000e-01),
		new RGBColor(6.200000e-01, 1, 3.800000e-01),
		new RGBColor(6.400000e-01, 1, 3.600000e-01),
		new RGBColor(6.600000e-01, 1, 3.400000e-01),
		new RGBColor(6.800000e-01, 1, 3.200000e-01),
		new RGBColor(7.000000e-01, 1, 3.000000e-01),
		new RGBColor(7.200000e-01, 1, 2.800000e-01),
		new RGBColor(7.400000e-01, 1, 2.600000e-01),
		new RGBColor(7.600000e-01, 1, 2.400000e-01),
		new RGBColor(7.800000e-01, 1, 2.200000e-01),
		new RGBColor(8.000000e-01, 1, 2.000000e-01),
		new RGBColor(8.200000e-01, 1, 1.800000e-01),
		new RGBColor(8.400000e-01, 1, 1.600000e-01),
		new RGBColor(8.600000e-01, 1, 1.400000e-01),
		new RGBColor(8.800000e-01, 1, 1.200000e-01),
		new RGBColor(9.000000e-01, 1, 1.000000e-01),
		new RGBColor(9.200000e-01, 1, 8.000000e-02),
		new RGBColor(9.400000e-01, 1, 6.000000e-02),
		new RGBColor(9.600000e-01, 1, 4.000000e-02),
		new RGBColor(9.800000e-01, 1, 2.000000e-02),
		new RGBColor(1, 1, 0),
		new RGBColor(1, 9.800000e-01, 0),
		new RGBColor(1, 9.600000e-01, 0),
		new RGBColor(1, 9.400000e-01, 0),
		new RGBColor(1, 9.200000e-01, 0),
		new RGBColor(1, 9.000000e-01, 0),
		new RGBColor(1, 8.800000e-01, 0),
		new RGBColor(1, 8.600000e-01, 0),
		new RGBColor(1, 8.400000e-01, 0),
		new RGBColor(1, 8.200000e-01, 0),
		new RGBColor(1, 8.000000e-01, 0),
		new RGBColor(1, 7.800000e-01, 0),
		new RGBColor(1, 7.600000e-01, 0),
		new RGBColor(1, 7.400000e-01, 0),
		new RGBColor(1, 7.200000e-01, 0),
		new RGBColor(1, 7.000000e-01, 0),
		new RGBColor(1, 6.800000e-01, 0),
		new RGBColor(1, 6.600000e-01, 0),
		new RGBColor(1, 6.400000e-01, 0),
		new RGBColor(1, 6.200000e-01, 0),
		new RGBColor(1, 6.000000e-01, 0),
		new RGBColor(1, 5.800000e-01, 0),
		new RGBColor(1, 5.600000e-01, 0),
		new RGBColor(1, 5.400000e-01, 0),
		new RGBColor(1, 5.200000e-01, 0),
		new RGBColor(1, 5.000000e-01, 0),
		new RGBColor(1, 4.800000e-01, 0),
		new RGBColor(1, 4.600000e-01, 0),
		new RGBColor(1, 4.400000e-01, 0),
		new RGBColor(1, 4.200000e-01, 0),
		new RGBColor(1, 4.000000e-01, 0),
		new RGBColor(1, 3.800000e-01, 0),
		new RGBColor(1, 3.600000e-01, 0),
		new RGBColor(1, 3.400000e-01, 0),
		new RGBColor(1, 3.200000e-01, 0),
		new RGBColor(1, 3.000000e-01, 0),
		new RGBColor(1, 2.800000e-01, 0),
		new RGBColor(1, 2.600000e-01, 0),
		new RGBColor(1, 2.400000e-01, 0),
		new RGBColor(1, 2.200000e-01, 0),
		new RGBColor(1, 2.000000e-01, 0),
		new RGBColor(1, 1.800000e-01, 0),
		new RGBColor(1, 1.600000e-01, 0),
		new RGBColor(1, 1.400000e-01, 0),
		new RGBColor(1, 1.200000e-01, 0),
		new RGBColor(1, 1.000000e-01, 0),
		new RGBColor(1, 8.000000e-02, 0),
		new RGBColor(1, 6.000000e-02, 0),
		new RGBColor(1, 4.000000e-02, 0),
		new RGBColor(1, 2.000000e-02, 0),
		new RGBColor(1, 0, 0),
		new RGBColor(9.800000e-01, 0, 0),
		new RGBColor(9.600000e-01, 0, 0),
		new RGBColor(9.400000e-01, 0, 0),
		new RGBColor(9.200000e-01, 0, 0),
		new RGBColor(9.000000e-01, 0, 0),
		new RGBColor(8.800000e-01, 0, 0),
		new RGBColor(8.600000e-01, 0, 0),
		new RGBColor(8.400000e-01, 0, 0),
		new RGBColor(8.200000e-01, 0, 0),
		new RGBColor(8.000000e-01, 0, 0),
		new RGBColor(7.800000e-01, 0, 0),
		new RGBColor(7.600000e-01, 0, 0),
		new RGBColor(7.400000e-01, 0, 0),
		new RGBColor(7.200000e-01, 0, 0),
		new RGBColor(7.000000e-01, 0, 0),
		new RGBColor(6.800000e-01, 0, 0),
		new RGBColor(6.600000e-01, 0, 0),
		new RGBColor(6.400000e-01, 0, 0),
		new RGBColor(6.200000e-01, 0, 0),
		new RGBColor(6.000000e-01, 0, 0),
		new RGBColor(5.800000e-01, 0, 0),
		new RGBColor(5.600000e-01, 0, 0),
		new RGBColor(5.400000e-01, 0, 0),
		new RGBColor(5.200000e-01, 0, 0),
		new RGBColor(5.000000e-01, 0, 0),
		new RGBColor(0, 0, 0) // add a black color.

	};

	std::vector<RGBColor*> jet(jetArray,
		jetArray + sizeof(jetArray) / sizeof(jetArray[0]));


	this->_colorMap = vtkLookupTable::New();
	this->_colorMap->SetNumberOfColors(jet.size());
	int idx = 0;
	for (int i = jet.size() - 1; i >= 0; i--)
		this->_colorMap->SetTableValue(idx++, jet.at(i)->red,
		jet.at(i)->green,
		jet.at(i)->blue, 0.5);

}

vtkLookupTable* JetColorMap::GetColorMap()
{
	if (_colorMapHandle == NULL)
		_colorMapHandle = new JetColorMap();
	return _colorMapHandle->_colorMap;
}

JetColorMap::~JetColorMap(void)
{
}