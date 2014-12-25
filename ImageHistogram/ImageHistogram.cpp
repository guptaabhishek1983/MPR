// ImageHistogram.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>

#include "rad_util.h"
#include "rad_logger.h"
#define __FILENAME__ "ImageHistogram.cpp"
#undef __MODULENAME__
#define __MODULENAME__ "ImageHistogram"

#include "RTSS.h"
#include "Dose.h"
#include "DCMGeometry.h"
#include "DVHComputation.h"

// vtk includes
#include "vtkSmartPointer.h"
#include "vtkImageData.h"
#include "vtkPolyDataToImageStencil.h"
#include "vtkImageStencil.h"
#include "vtkImageAccumulate.h"
#include "vtkImageHistogram.h"
#include "vtkImageHistogramStatistics.h"
#include "vtkXYPlotActor.h"
#include "vtkMath.h"
#include "vtkFieldData.h"
#include "vtkDataArray.h"
#include "vtkAbstractArray.h"

#include "vtkAutoInit.h"
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);
VTK_MODULE_INIT(vtkRenderingFreeTypeOpenGL);
VTK_MODULE_INIT(vtkRenderingOpenGL);

using namespace std;
using namespace RTViewer;

#include <functional>
#include <algorithm>
#include <numeric>

int _tmain(int argc, _TCHAR* argv[])
{
	vector<float> a;
	for (int i = 0; i < 5; i++)
	{
		a.push_back(i/10);
	}
	int t = std::accumulate(a.begin(), a.end(), 0);

	string logFile = "C:\\Temp\\Histogram.log";
	DeleteFile((LPCWSTR)logFile.c_str());
	rad_setLogFileName(logFile.c_str());
	rad_setLogLevel(7);

	double origin[3] = { -247.5, -247.5, 22 };
	double spacing[3] = { 0.966797, 0.966797, -2.5 };
	double xOrient[3] = { 1, 0, 0 };
	double yOrient[3] = { 0, 1, 0 };
	int dim[3] = { 512, 512, 163 };
	/*double origin[3] = { -275.00000000000000, -524.00000000000000, 168.55930000000001 };
	double spacing[3] = { 1.0742190000000000, 1.0742190000000000, -3.0000000000000000 };
	double xOrient[3] = { 1.0000000000000000, 0.00000000000000000, 1.2246469999999999e-016 };
	double yOrient[3] = { 0.00000000000000000, 1.0000000000000000, 0.00000000000000000 };
	int dim[3] = { 512, 512, 98 };*/
	RAD_LOG_CRITICAL("Loading structure set");
	RadRTDicomInterface* pDicom = new RTDcmtkDicomInterface("D:\\DicomDataSet\\HCG-Dicoms\\StreamingData\\ESHWAR_GOWDA\\RTSS\\rtss.dcm");

	RTStructureSet* m_rtss = new RTStructureSet((RTDcmtkDicomInterface*)pDicom);
	m_rtss->ComputeSlicers(origin, spacing, xOrient, yOrient, dim);

	RAD_LOG_CRITICAL("Loading plan");
	RadRTDicomInterface* planDicom = new RTDcmtkDicomInterface("D:\\DicomDataSet\\HCG-Dicoms\\StreamingData\\ESHWAR_GOWDA\\RTPLAN\\RTPLAN.dcm");

	Plan* m_plan = new Plan((RTDcmtkDicomInterface*)planDicom);

	RAD_LOG_CRITICAL("Loading dose");
	RadRTDicomInterface* doseDicom = new RTDcmtkDicomInterface("D:\\DicomDataSet\\HCG-Dicoms\\StreamingData\\ESHWAR_GOWDA\\RTDOSE\\RTDOSE.dcm");

	Dose* m_dose = new Dose((RTDcmtkDicomInterface*)doseDicom, m_plan);

	DVHComputation* m_dvh_comp = new DVHComputation();
	m_dvh_comp->Init(m_rtss->getStructureSetROISequences(1)->getReslicer(), m_dose);
	m_dvh_comp->ComputeHistogram(0.01);
	m_dvh_comp->WriteToCSV("D:\\computed_dvh.csv");

	m_dvh_comp->Init(m_rtss->getStructureSetROISequences(2)->getReslicer(), m_dose);
	m_dvh_comp->ComputeHistogram(0.01);
	m_dvh_comp->WriteToCSV("D:\\computed_dvh_2.csv");

	//fieldData->AllocateArrays(volBin.size());


	////// Create a vtkXYPlotActor
	//vtkSmartPointer<vtkXYPlotActor> plot =	vtkSmartPointer<vtkXYPlotActor>::New();
	//plot->ExchangeAxesOff();
	//plot->SetLabelFormat("%g");
	//plot->SetXTitle("Dose(%)");
	//plot->SetYTitle("Vol(cc)");
	//plot->SetXValuesToValue();
	//plot->AddDataSetInput(stencil2->GetOutputPort());
	//plot->SetPlotColor(0, 255, 0, 0);

	//plot->SetXRange(0, xMax);
	//plot->SetYRange(0, yMax);
	//// Visualize
	//vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();

	//vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
	//renderWindow->AddRenderer(renderer);

	//vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	//renderWindowInteractor->SetRenderWindow(renderWindow);

	//renderer->AddActor(plot);
	//renderWindow->Render();
	//renderWindowInteractor->Start();

	return 0;
}



