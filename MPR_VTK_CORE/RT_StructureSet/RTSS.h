#pragma once
#include "RTDcmtkDicomInterface.h"
# include "vtkRTSSMPR.h"
#include "..\enums.h"
using namespace std;
/*******************************************
RT STRUCTURE SET STORE
*******************************************/
class vtkPoints; // forward decalaration;
class vtkTransform;

namespace RTViewer
{
	struct ROIColor
	{
		int roiNumber;
		int r, g, b;
		ROIColor()
		{
		}
	};
	struct ROIPlotPoints
	{
		float x;
		float y;
		float z;
		int roiNumber;
		ROIPlotPoints()
		{

		}
	};
	class ROIContourSequence
	{
	public:
		struct Contour3DPoint{
		public:
			double x;
			double y;
			double z;
		};
	public:
		~ROIContourSequence();
		void setNumberOfContourPoints(int number)	{ _numberOfContourPoints = number; }
		void AddContourPoint(Contour3DPoint point)	{ _contourPoints.push_back(point); }
		vector<Contour3DPoint> getContourPoints()	{ return _contourPoints; }
		void UpdateContourPoints(int idx, double x, double y, double z);
		string getReferencedCTImageSOPUID()			{ return _referencedCTImageSOPUID; }
	private:
		string _referencedCTImageSOPUID;
		int _numberOfContourPoints;
		vector<Contour3DPoint> _contourPoints;

		vtkPoints* _contourPointsVTK;

		friend class RTStructureSet;
	protected:
		ROIContourSequence();
		ROIContourSequence(string referencedCTImageSOPUID);
	};

	class StructureSetROISequence
	{
	public:
		~StructureSetROISequence();
		void setColor(int r, int g, int b)	{ _rColor = r;	_gColor = g;	_bColor = b; }
		void setName(string name)			{ _roiName = name; }
		void setNumber(int number)			{ _roiNumber = number; }
		void setSelected(bool selected)			{ _selected = selected; }
		int getNumber()						{ return _roiNumber; }
		string getName()					{ return _roiName; }
		int getRColor()						{ return _rColor; }
		int getGColor()						{ return _gColor; }
		int getBColor()						{ return _bColor; }
		bool getSelected()						{ return _selected; }
		vtkRTSSMPR* getReslicer()						{ return Reslicer; }
		ROIContourSequence* getROIContourSequence(string referencedCTSOPInstanceUID);
		vector<ROIContourSequence*> getROIContourSequenceVector(string referencedCTSOPInstanceUID);
		void addROIContourSequence(ROIContourSequence* roiContourSequence){ _roiContourSequences.push_back(roiContourSequence); }
	private:
		int _rColor;
		int _gColor;
		int _bColor;
		string _roiName;
		int _roiNumber;
		bool _selected;
		vector<ROIContourSequence*> _roiContourSequences;
		vtkRTSSMPR* Reslicer;
		friend class RTStructureSet;
	protected:
		StructureSetROISequence();
	};

	class RTStructureSet
	{
	public:
		RTStructureSet(void);
		RTStructureSet(RTDcmtkDicomInterface* rtStructureSet);
		~RTStructureSet(void);
		string getFrameRefUID() { return frameRefUID; }
		string getStudyUID() { return studyUID; }
		string getSopUID() { return sopUID; }
		string getSeriesUID() { return seriesUID; }
		string getRefCTSeriesUID() { return referencedCTSeriesUID; }
		map<int, StructureSetROISequence*> getStructureSetROISequences(){ return structureSetROISequences; }
		StructureSetROISequence* getStructureSetROISequences(int roiNumber){ return structureSetROISequences[roiNumber]; }
		bool IsComputed(){ return computed; }

		void ComputeSlicers(double* origin, double* spacing, double* directionX, double* directionY, int* dim);

		void ComputeROI(int axis, double pos, double firstImagePosition[3], double lastImagePosition[3], vector<vector<vector<ROIPlotPoints>>>& outPoints, vector<ROIColor>& outColors);

	private:
		string studyUID;
		string sopUID;
		string seriesUID;
		string frameRefUID;
		string referencedCTSeriesUID;
		bool computed;
		map<int, StructureSetROISequence*> structureSetROISequences;
	};



}