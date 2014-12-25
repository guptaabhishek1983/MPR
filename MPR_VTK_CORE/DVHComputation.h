#pragma once
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkImageData.h"
#include "vtkPolyDataToImageStencil.h"
#include "vtkImageAccumulate.h"
#include "vtkImageStencil.h"

#include "DCMGeometry.h"
#include "enums.h"
#include "vtkRTSSMPR.h"
#include "Dose.h"
#define NUMBER_OF_DVH_BINS 1000
using namespace std;

namespace RTViewer
{
	struct DVHComputationData;
	class DVHComputation
	{
	public:
		DVHComputation();
		~DVHComputation();

		void Init(vtkRTSSMPR* rtssMPR, Dose* rtDose);
		void ComputeHistogram(double doseBinWidth);
		void WriteToCSV(string fileName);
		void WriteToCSV2(string fileName);
		vector<double> GetFrequncyBins();
		vector<double> GetVolumeBins();
		double GetMaxDoseValue(DoseUnit unit);
		double GetMinDoseValue(DoseUnit unit);
		double GetMeanDoseValue(DoseUnit unit);

	private:
		void ComputeHistogram2(int numberOfBins, DVHType dvhType);
		void getStructBoundsInMatrixCoords(double imgBounds[6], double structBounds[6], double xSpc, double ySpc, double zSpc, int xDims, int yDims, int zDims);
		void getIndicesOfOnes(vtkImageData* imgData);
		void getDoseValues(std::vector<int>, std::vector<int>, std::vector<int>, vtkImageData* doseImage);
		void ComputeCumulative();

	private:
		DVHComputationData* d;
		double structBoundsInMatrixCoords[6];
		std::vector<int>indicesX;
		std::vector<int>indicesY;
		std::vector<int>indicesZ;

	};
}
