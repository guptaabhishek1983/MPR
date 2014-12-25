#pragma once
#include "RTDcmtkDicomInterface.h"
using namespace std;
#include "vtkSmartPointer.h"

#include "enums.h"
class vtkImageData;
class vtkLookupTable;

//class MPRSlicer;
class DCMGeometry;

namespace RTViewer
{
	class DVHComputation;
	class StructureSetROISequence;
	struct DoseReferenceSequence;
	struct doseFrameData;
	//class MPRTransform;
	struct dosePlotPoint
	{
		float x;
		float y;
		float z;
		int level;
		dosePlotPoint()
		{

		}
	};

	struct isodose
	{
		int level;
		int rgbColor[3];
		string name;
		bool checked;
		int opacity;
		bool fill;
		int cGyValue;
		double pixelIntensity;
		string Type;

		isodose(int _level, int _rColor, int _gColor, int _bColor, string _name, bool _checked, int _cGyValue, double _pixelIntensity, string _type)
		{
			this->level = _level;
			this->rgbColor[0] = _rColor;
			this->rgbColor[1] = _gColor;
			this->rgbColor[2] = _bColor;
			this->name = _name;
			this->checked = _checked;
			this->cGyValue = _cGyValue;
			this->pixelIntensity = _pixelIntensity;
			this->Type = _type;
		} // default ctor
	};
	
	
	struct RGBColor
	{

		double red;
		double green;
		double blue;

		RGBColor(double _r, double _g, double _b)
		{
			red = _r;
			green = _g;
			blue = _b;
		}
	};
	class JetColorMap
	{
	protected:
		JetColorMap(void);

	public:
		~JetColorMap(void);
		static vtkLookupTable* GetColorMap();
	private:
		static JetColorMap* _colorMapHandle;
		vtkLookupTable* _colorMap;
	};
	
	class Plan
	{

	public:
		Plan(RTDcmtkDicomInterface* rtPlanDicom);
		~Plan();

		float getTargetPrescribedDose();
	private:
		string studyUID;
		string sopUID;
		string frameRefUID;
		string rtPlanLabel;
		float rxDose;
		std::vector<DoseReferenceSequence*> DoseReferenceSequences;
	};


	class DVH
	{
	private:
		double DVHMinDose;
		double DVHMaxDose;
		double DVHMeanDose;
		int refROINumber;
		string DVHVolume;
		string DVHData;
		string strucSetRefSopUID;
		string refPlanSOPUID;
		string refDoseSOPUID;
		double maxDVHValue;

	public:
		vector<double>dvhDataList;
		void setStrucSetRefSopUID(string refSOPUID)
		{
			strucSetRefSopUID = refSOPUID;
		}
		string getSopUID()
		{
			return strucSetRefSopUID;
		}
		void setROINumber(int ROINumber)
		{
			refROINumber = ROINumber;
		}
		int getROINumber()
		{
			return refROINumber;
		}
		void setDVHMiniDose(double miniDose)
		{
			DVHMinDose = miniDose;
		}
		double getDVHMiniDose()
		{
			return DVHMinDose;
		}
		void setDVHMaxDose(double maxDose)
		{
			DVHMaxDose = maxDose;
		}
		double getDVHMaxDose()
		{
			return DVHMaxDose;
		}
		void setDVHMeanDose(double meanDose)
		{
			DVHMeanDose = meanDose;
		}
		double getDVHMeanDose()
		{
			return DVHMeanDose;
		}
		void setDVHVolume(string volume)
		{
			DVHVolume = volume;
		}
		string getDVHVolume()
		{
			return DVHVolume;
		}
		void setDVHData(string data)
		{
			DVHData = data;
		}

		double getMaxDVHValue()
		{
			return maxDVHValue;
		}
		void setMaxDVHValue(double value)
		{
			maxDVHValue = value;
		}

		void setRefPlanSOPUID(string value)
		{
			refPlanSOPUID = value;
		}

		string getRefPlanSOPUID(){ return refPlanSOPUID; }



	};

	struct DoseData;
	class Dose
	{
	public:
		Dose(RTDcmtkDicomInterface* rtDoseDicom, Plan* rtPlan);
		~Dose();

		void InitMPRPipeline();
		vtkSmartPointer<vtkImageData> ComputeColorWash(int axis, double pos, double& translateX, double& translateY);
		vtkSmartPointer<vtkImageData> ComputeColorWash2(int axis, int idx, double& translateX, double& translateY);
		void Scroll(Axis axis, int delta);
		void Scroll2(Axis axis, float dx, float dy);
		void Scroll3(Axis axis, double pos);
		DCMGeometry* GetDCMGeometry();
		
		int GetCurrentImageIndex(Axis axis);
		double GetCurrentImagePosition(Axis axis);
		void ComputeROI(int axis, int idx, double fip[3], double lip[3], vector<vector<vector<dosePlotPoint>> >& outPoints);
		vtkImageData* GetVolume();
		vtkImageData* Resample(double sx, double sy, double sz);
		vtkImageData* GetResampledVolume();
		DCMGeometry* GetResampledDCMGeometry(); 
		vector<DVH*> dh;
		vector<DVH*> computed_dvh;
		float rxDose;
		double GetDoseGridScaling(){ return doseGridScaling; }
		void ComputeDVH(StructureSetROISequence* roiSequence);
	private:
		int width, height;
		string frameRefUID, studyUID, seriesUID, sopUID;
		double minDosePixelValue, maxDosePixelValue, doseGridScaling;
		bool isMultiframe;
		vector<double> doseGridOffsetVector;

		

		vector<doseFrameData*> allFrameDoseData;
		vector<isodose*> isodoses;

		

		DoseData* d;

	};

}