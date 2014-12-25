#include "DVHComputation.h"
#include "rad_logger.h"
#include "rad_template_helper.h"
#include "rad_util.h"

#define __FILENAME__ "DoseComputation.cpp"
#undef  __MODULENAME__
#define __MODULENAME__ "DoseComputation"

#include <iostream>
#include <vector>
#include <functional>
#include <algorithm>
#include <numeric>
#include "vtkMath.h"

using namespace RTViewer;
using namespace std;
namespace RTViewer
{
	struct DVHComputationData
	{
		std::vector<double> doseValues;
		vector<double> doseBin;
		vector<double> frequencyBin;
		vector<double> volBin_cc;

		vector<double> cumulativeVolBin;

		float totalVol_cc;
		float maxDosePercent;
		float minDosePercent;
		float meanDosePercent;
		float stdDevDosePercent;

		float maxDoseGy;
		float minDoseGy;
		float meanDoseGy;
		float stdDevDoseGy;

		float maxDose_cGy;
		float minDose_cGy;
		float meanDose_cGy;
		float stdDevDose_cGy;

		float voxelVol_cc;
		vtkIdType numOfVoxels;
	};
}

DVHComputation::DVHComputation()
{
	d = new DVHComputationData();
}


DVHComputation::~DVHComputation()
{
}

void DVHComputation::Init(vtkRTSSMPR* rtssMPR, Dose* rtDose)
{
	vtkPolyData* structureGrid = rtssMPR->GetSurface();
	DCMGeometry* doseGeometry = rtDose->GetDCMGeometry();
	vtkImageData* doseImage = rtDose->GetVolume();

	double bounds[6];
	structureGrid->GetBounds(bounds);
	double doseBounds[6];
	doseGeometry->GetBounds(doseBounds);
	int doseDims[3];
	doseGeometry->GetDimensions(doseDims);
	double doseSpc[3];
	doseGeometry->GetSpacing(doseSpc);
	double doseOrgn[3];
	doseGeometry->GetOrigin(doseOrgn);
	int doseExtent[6];
	doseGeometry->GetExtent(doseExtent);

	this->getStructBoundsInMatrixCoords(doseBounds, bounds, doseSpc[0], doseSpc[1], doseSpc[2], doseDims[0], doseDims[1], doseDims[2]);


	vtkPolyDataToImageStencil* sts2 =
		vtkPolyDataToImageStencil::New();
	sts2->SetTolerance(0);//very important
	sts2->SetInputData(structureGrid);
	sts2->SetOutputSpacing(doseSpc);
	sts2->SetOutputOrigin(doseOrgn);
	sts2->SetOutputWholeExtent(doseExtent);
	sts2->Update();


	vtkImageStencil* stencil2 = vtkImageStencil::New();
	stencil2->SetStencilData(sts2->GetOutput());
	stencil2->SetInputData(doseImage);
	stencil2->ReverseStencilOff();
	stencil2->SetBackgroundValue(0);
	stencil2->Update();

	
	vtkImageAccumulate* ia2 = vtkImageAccumulate::New();
	ia2->SetInputConnection(stencil2->GetOutputPort());
	ia2->IgnoreZeroOn();
	ia2->GetOutput()->ReleaseData();
	ia2->Update();
	
	double stencilSpacing[3];
	stencil2->GetOutput()->GetSpacing(stencilSpacing);

	d->numOfVoxels = ia2->GetVoxelCount();
	d->voxelVol_cc = (stencilSpacing[0] * stencilSpacing[1] * stencilSpacing[2]) / 1000;//in cc
	d->totalVol_cc = (d->numOfVoxels *d->voxelVol_cc);//in cc

	d->maxDosePercent = ia2->GetMax()[0] / 10000;
	d->minDosePercent = ia2->GetMin()[0] / 10000;
	d->meanDosePercent = ia2->GetMean()[0] / 10000;
	d->stdDevDosePercent = ia2->GetStandardDeviation()[0]/10000;
	
	d->maxDoseGy = ia2->GetMax()[0] * rtDose->GetDoseGridScaling();
	d->minDoseGy = ia2->GetMin()[0] * rtDose->GetDoseGridScaling();
	d->meanDoseGy = ia2->GetMean()[0] * rtDose->GetDoseGridScaling();
	d->stdDevDoseGy = ia2->GetStandardDeviation()[0] * rtDose->GetDoseGridScaling();

	d->maxDose_cGy = d->maxDoseGy * 100;
	d->minDose_cGy = d->minDoseGy * 100;
	d->meanDose_cGy = d->meanDoseGy * 100;
	d->stdDevDose_cGy = d->stdDevDoseGy * 100;

	
	RAD_LOG_CRITICAL("Voxel count#" << d->numOfVoxels);
	RAD_LOG_CRITICAL("Volume[cc]:" << d->totalVol_cc);
	RAD_LOG_CRITICAL("Max Dose[%][Gy][cGy]:" << d->maxDosePercent << ","<<d->maxDoseGy << "," << d->maxDose_cGy);
	RAD_LOG_CRITICAL("Min Dose[%][Gy][cGy]:" << d->minDosePercent << "," << d->minDoseGy << "," << d->minDose_cGy);
	RAD_LOG_CRITICAL("Mean Dose[%][Gy][cGy]:" << d->meanDosePercent << "," << d->meanDoseGy << "," << d->meanDose_cGy);

	this->getIndicesOfOnes(stencil2->GetOutput());
	this->getDoseValues(this->indicesX, this->indicesY, this->indicesZ, stencil2->GetOutput());

	// convert dose values from pixel to Gy.
	std::transform(d->doseValues.begin(), d->doseValues.end(),
		d->doseValues.begin(), std::bind2nd(std::multiplies<double>(), rtDose->GetDoseGridScaling()));

	//ComputeHistogram2(0.01, DVHType::Differential);

	ia2->Delete();
	stencil2->Delete();
	sts2->Delete();
	return;
}


void DVHComputation::ComputeCumulative()
{
	
	double totalVol_cc = std::accumulate(d->volBin_cc.begin(), d->volBin_cc.end(), 0.0);

	d->cumulativeVolBin.resize(d->volBin_cc.size());
	d->cumulativeVolBin[0] = d->volBin_cc[0];

	for (int i = 1; i < d->volBin_cc.size(); ++i)
	{
		d->cumulativeVolBin[i] = d->cumulativeVolBin[i - 1] + d->volBin_cc[i];
	}
	std::reverse(d->cumulativeVolBin.begin(), d->cumulativeVolBin.end());
}


void DVHComputation::getIndicesOfOnes(vtkImageData* imgData)
{
	int dims[6];
	dims[0] = this->structBoundsInMatrixCoords[0];
	dims[1] = this->structBoundsInMatrixCoords[1];
	dims[2] = this->structBoundsInMatrixCoords[2];
	dims[3] = this->structBoundsInMatrixCoords[3];
	dims[4] = this->structBoundsInMatrixCoords[4];
	dims[5] = this->structBoundsInMatrixCoords[5];


	for (int x = dims[0]; x<dims[1]; x++)
	{
		for (int y = dims[2]; y<dims[3]; y++)
		{
			for (int z = dims[4]; z<dims[5]; z++)
			{
				double* pix = static_cast<double*>(imgData->GetScalarPointer(x, y, z));
				//double* pix=(double*)imgData->GetScalarPointer(x,y,z);
				//if(imgData->GetScalarComponentAsFloat(x,y,z,0)>0)
				if (pix[0]>0)
				{
					this->indicesX.push_back(x);
					this->indicesY.push_back(y);
					this->indicesZ.push_back(z);
					//qDebug()<<x<<y<<z<<pix[0];
				}

			}
		}
	}

}
void DVHComputation::getDoseValues(std::vector<int>, std::vector<int>, std::vector<int>, vtkImageData* doseImage)
{
	float* pixel;
	d->doseValues.clear();
	//x,y,z all have the same size, so just get the size of one
	for (int i = 0; i<indicesX.size(); i++)
	{
		pixel = static_cast<float*>(doseImage->GetScalarPointer(indicesX[i], indicesY[i], indicesZ[i]));
		//pixel=(double*)doseGrid->GetScalarPointer(indicesX[i],indicesY[i],indicesZ[i]);
		d->doseValues.push_back(pixel[0]);
	}
}

void DVHComputation::getStructBoundsInMatrixCoords(double imgBounds[6], double structBounds[6], double xSpc, double ySpc, double zSpc, int xDims, int yDims, int zDims)
{

	double x1 = imgBounds[0] - structBounds[0];
	int xStart = std::abs(vtkMath::Round(x1 / xSpc));
	int xEnd1 = std::abs(vtkMath::Round(imgBounds[1])) - (std::abs(vtkMath::Round(structBounds[1])));
	int xEnd = xDims - (std::abs(vtkMath::Round(xEnd1 / xSpc)));
	//qDebug()<<xStart<<xEnd<<"X1-X2";

	double y1 = imgBounds[2] - structBounds[2];
	int yStart = std::abs(vtkMath::Round((y1 / ySpc)));
	int yEnd1 = std::abs(vtkMath::Round(imgBounds[3])) - (std::abs(vtkMath::Round(structBounds[3])));
	int yEnd = yDims - (std::abs(vtkMath::Round(yEnd1 / ySpc)));
	//qDebug()<<yStart<<yEnd<<"Y1-Y2";

	double z1 = imgBounds[4] - structBounds[4];
	int zStart = std::abs(vtkMath::Round(z1 / zSpc));
	int zEnd1 = std::abs(vtkMath::Round(imgBounds[5])) - (std::abs(vtkMath::Round(structBounds[5])));
	int zEnd = zDims - (std::abs(vtkMath::Round(zEnd1 / zSpc)));
	//qDebug()<<zStart<<zEnd<<"Z1-Z2";

	//Add one voxel on either side for safety
	this->structBoundsInMatrixCoords[0] = xStart - 0;
	this->structBoundsInMatrixCoords[1] = xEnd + 0;
	this->structBoundsInMatrixCoords[2] = yStart - 0;
	this->structBoundsInMatrixCoords[3] = yEnd + 0;
	this->structBoundsInMatrixCoords[4] = zStart - 0;
	this->structBoundsInMatrixCoords[5] = zEnd + 0;

	RAD_LOG_CRITICAL("structBoundsInMatrixCoords: Xs:" << xStart << " Xe:" << xEnd);
	RAD_LOG_CRITICAL("structBoundsInMatrixCoords: Ys:" << yStart << " Ye:" << yEnd);
	RAD_LOG_CRITICAL("structBoundsInMatrixCoords: Zs:" << zStart << " Ze:" << zEnd);
}


void DVHComputation::ComputeHistogram2(int numberOfBins, DVHType dvhType)
{
	std::vector<double>frequency;
	frequency.resize(numberOfBins);
	std::fill(frequency.begin(), frequency.end(), 0.0);
	int dataSize = d->doseValues.size();
	
	double doseMax = *std::max_element(d->doseValues.begin(), d->doseValues.end());//pointer is important
	double doseMin = *std::min_element(d->doseValues.begin(), d->doseValues.end());//pointer is important

	double doseInterval = doseMax - doseMin;
	double eachBinSize = doseInterval / numberOfBins;
	
	double min, max, binCentre;
	for (int i = 0; i<numberOfBins; ++i)
	{
		RAD_LOG_CRITICAL("Computing bin#" << i);
		min = (i*eachBinSize) + doseMin;
		max = (i*eachBinSize) + (eachBinSize + doseMin);
		binCentre = min + (max - min) / 2.0;
		for (int k = 0; k<dataSize; k++)
		{
			//to avoid rounding off errors,(sometimes the max dose will be 0.0001 less than the actual max dose
			//and voxels in a more uniform dose distribution will be avoided)
			if (i == (numberOfBins - 1))//last bin
			{
				max = doseMax;
			}
			if (d->doseValues[k] > min && d->doseValues[k] < max)
			{
				frequency[i]++;
			}
		}

		d->doseBin.push_back(binCentre);
		if (dvhType == DVHType::Cumulative)//Cumulative
		{
			d->frequencyBin.push_back(frequency[i] * d->voxelVol_cc);
		}
		else if (dvhType == DVHType::Differential)//Differential
		{
			d->frequencyBin.push_back(frequency[i]);
		}
	}
}

void DVHComputation::ComputeHistogram(double doseBinWidth)
{ 
	//doseBinWidth in Gy
	if (d->numOfVoxels==0)
		return;
	RAD_LOG_CRITICAL("Computing histogram");
	double doseMax = *std::max_element(d->doseValues.begin(), d->doseValues.end());//pointer is important
	double doseMin = *std::min_element(d->doseValues.begin(), d->doseValues.end());//pointer is important

	double doseInterval = (doseMax - doseMin);
	int numOfBins = vtkMath::Round(doseInterval / doseBinWidth);
	if (numOfBins<10)
	{
		numOfBins = 10;
	}
	RAD_LOG_CRITICAL("Number of bins:" << numOfBins);
	RAD_LOG_CRITICAL("BEGIN: Computing Differential DVH");

	this->ComputeHistogram2(numOfBins, DVHType::Differential);
	RAD_LOG_CRITICAL("DONE: Computing Differential DVH");
	std::reverse(d->frequencyBin.begin(), d->frequencyBin.end());
	d->volBin_cc = d->frequencyBin;
	std::transform(d->volBin_cc.begin(), d->volBin_cc.end(), d->volBin_cc.begin(), std::bind2nd(std::multiplies<double>(), d->voxelVol_cc));

	RAD_LOG_CRITICAL("Begin: Computing Cumulative DVH");
	this->ComputeCumulative();
	RAD_LOG_CRITICAL("DONE: Computing Cumulative DVH");
}

void DVHComputation::WriteToCSV(string fileName)
{
	ofstream Morison_File(fileName.c_str());
	Morison_File << "#Voxels, dose[Gy]"<<endl;
	for (int i = 0; i < d->frequencyBin.size(); i++)
	{
		Morison_File << d->frequencyBin.at(i) << "," << d->doseBin.at(i) << endl;
	}
	Morison_File.close();
}


void DVHComputation::WriteToCSV2(string fileName)
{
	ofstream Morison_File(fileName.c_str());
	Morison_File << "Vol[cc], dose[Gy]" << endl;

	for (int i = 0; i < d->cumulativeVolBin.size(); i++)
	{
		Morison_File << d->cumulativeVolBin.at(i) << "," << d->doseBin.at(i) << endl;
	}
	Morison_File.close();
}

vector<double> DVHComputation::GetFrequncyBins()
{
	return d->frequencyBin;
}

vector<double> DVHComputation::GetVolumeBins()
{
	return d->cumulativeVolBin;
}

double DVHComputation::GetMaxDoseValue(DoseUnit unit)
{
	double val = 0;
	switch(unit)
	{	
		case DoseUnit::cGy:
			val = d->maxDose_cGy;
			break;
		case DoseUnit::Gy:
			val = d->maxDoseGy;
			break;
		case DoseUnit::Percent:
			val = d->maxDosePercent;
				break;

		default:
			break;
	}
	return val;
}
double DVHComputation::GetMinDoseValue(DoseUnit unit){
	double val = 0;
	switch (unit)
	{
		case DoseUnit::cGy:
			val = d->minDose_cGy;
			break;
		case DoseUnit::Gy:
			val = d->minDoseGy;
			break;
		case DoseUnit::Percent:
			val = d->minDosePercent;
			break;

		default:
			break;
	}
	return val;
}
double DVHComputation::GetMeanDoseValue(DoseUnit unit){
	double val = 0;
	switch (unit)
	{
		case DoseUnit::cGy:
			val = d->meanDose_cGy;
			break;
		case DoseUnit::Gy:
			val = d->meanDoseGy;
			break;
		case DoseUnit::Percent:
			val = d->meanDosePercent;
			break;

		default:
			break;
	}
	return val;
}