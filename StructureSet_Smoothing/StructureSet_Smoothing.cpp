// StructureSet_Smoothing.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "rad_util.h"
#include "rad_logger.h"
#define __FILENAME__ "StructSet_Smoothing.cpp"
#undef __MODULENAME__
#define __MODULENAME__ "StructSet_Smoothing"

#include "RTSS.h"
#include "vtkRTSSMPR.h"

// vtk includes
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkPlane.h"

#include "vtkPoints.h"


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
int _tmain(int argc, _TCHAR* argv[])
{
	vector<int> a = { 1, 2, 3, 4 };
	std::stringstream result;
	std::copy(a.begin(), a.end(), std::ostream_iterator<int>(result, ","));

	string t = result.str().c_str();
	t = t.substr(0, t.length() - 1);

	string logFile = "C:\\Temp\\Smoothing.log";
	DeleteFile((LPCWSTR)logFile.c_str());
	rad_setLogFileName(logFile.c_str());
	rad_setLogLevel(7);

	double origin[3] = { -247.5, -247.5, 22 };
	double spacing[3] = { 0.966797, 0.966797, -2.5 };
	double xOrient[3] = { 1, 0, 0 };
	double yOrient[3] = { 0, 1, 0 };
	int dim[3] = { 512, 512, 163 };

	RAD_LOG_CRITICAL("Loading structure set");
	RadRTDicomInterface* pDicom = new RTDcmtkDicomInterface("D:\\DicomDataSet\\HCG-Dicoms\\StreamingData\\ESHWAR_GOWDA\\RTSS\\rtss.dcm");

	RTStructureSet* m_rtss = new RTStructureSet((RTDcmtkDicomInterface*)pDicom);
	m_rtss->ComputeSlicers(origin, spacing, xOrient, yOrient, dim);

	StructureSetROISequence* roiSequence = m_rtss->getStructureSetROISequences(19);
	vtkRTSSMPR* rtssMPR =  roiSequence->getReslicer();

	vtkPlane* plane = vtkPlane::New();
	plane->SetNormal(0, 0, 1); // set plane normal to X-Axis
	plane->SetOrigin(0, 0, -64.5);// set plane origin where Y and Z points are 0. 

	vtkSmartPointer< vtkPolyData > surface = rtssMPR->GetCut(plane);
	
	//interpolate
	vtkCellArray* verts = surface->GetLines();
	vtkPoints* pPoints = surface->GetPoints();


	//vtkShepardMethod* shephard = vtkShepardMethod::New();
	//shephard->SetInputData(surface);

	vtkPolyDataMapper* dataMapper = vtkPolyDataMapper::New();
	dataMapper->SetInputData(surface);
	dataMapper->Update();

	vtkActor* actor = vtkActor::New();
	actor->SetMapper(dataMapper);
	actor->GetProperty()->SetColor(255, 0, 0);
	
	// Visualize
	vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();

	vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
	renderWindow->AddRenderer(renderer);

	vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	renderWindowInteractor->SetRenderWindow(renderWindow);

	//TODO: Add actors
	renderer->AddActor(actor);

	renderWindow->Render();
	renderWindowInteractor->Start();

	return 0;
}

