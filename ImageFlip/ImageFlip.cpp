// ImageFlip.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkDICOMImageReader.h>
#include <vtkImageFlip.h>
#include <vtkImageReslice.h>
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkActor.h>
#include <vtkLookupTable.h>
#include <vtkImageMapToWindowLevelColors.h>
#include <vtkImageActor.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkInteractorStyleImage.h>
#include <vtkCommand.h>
#include <vtkImageSliceMapper.h>
#include <vtkImageChangeInformation.h>

#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);
VTK_MODULE_INIT(vtkRenderingFreeTypeOpenGL);
VTK_MODULE_INIT(vtkRenderingOpenGL);
class vtkImageInteractionCallback : public vtkCommand
{
public:

	static vtkImageInteractionCallback *New() {
		return new vtkImageInteractionCallback;
	};

	vtkImageInteractionCallback() {
		this->Slicing = 0;
		this->ImageReslice = 0;
		this->Interactor = 0;
	};

	void SetImageReslice(vtkSmartPointer<vtkImageReslice> reslice) {
		this->ImageReslice = reslice;
	};

	vtkSmartPointer<vtkImageReslice> GetImageReslice() {
		return this->ImageReslice;
	};

	void SetInteractor(vtkSmartPointer<vtkRenderWindowInteractor> interactor) {
		this->Interactor = interactor;
	};

	vtkSmartPointer<vtkRenderWindowInteractor> GetInteractor() {
		return this->Interactor;
	};

	virtual void Execute(vtkObject *, unsigned long event, void *)
	{
		vtkSmartPointer<vtkRenderWindowInteractor> interactor = this->GetInteractor();

		int lastPos[2];
		interactor->GetLastEventPosition(lastPos);
		int currPos[2];
		interactor->GetEventPosition(currPos);

		if (event == vtkCommand::LeftButtonPressEvent)
		{
			this->Slicing = 1;
		}
		else if (event == vtkCommand::LeftButtonReleaseEvent)
		{
			this->Slicing = 0;
		}
		else if (event == vtkCommand::MouseMoveEvent)
		{
			if (this->Slicing)
			{
				vtkImageReslice *reslice = this->ImageReslice;

				// Increment slice position by deltaY of mouse
				int deltaY = lastPos[1] - currPos[1];

				
				double sliceSpacing = reslice->GetOutput()->GetSpacing()[2];
				vtkMatrix4x4 *matrix = reslice->GetResliceAxes();
				// move the center point that we are slicing through
				double point[4];
				double center[4];
				point[0] = 0.0;
				point[1] = 0.0;
				point[2] = sliceSpacing * deltaY;
				point[3] = 1.0;
				matrix->MultiplyPoint(point, center);
				matrix->SetElement(0, 3, center[0]);
				matrix->SetElement(1, 3, center[1]);
				matrix->SetElement(2, 3, center[2]);
				cout << center[0] << ":" << center[1] << ":" << center[2] << endl;
				reslice->Update();
				interactor->Render();
			}
			else
			{
				vtkInteractorStyle *style = vtkInteractorStyle::SafeDownCast(
					interactor->GetInteractorStyle());
				if (style)
				{
					style->OnMouseMove();
				}
			}
		}
	};

private:

	// Actions (slicing only, for now)
	int Slicing;

	// Pointer to vtkImageReslice
	vtkSmartPointer<vtkImageReslice> ImageReslice;

	// Pointer to the interactor
	vtkSmartPointer<vtkRenderWindowInteractor> Interactor;
};

int _tmain(int argc, _TCHAR* argv[])
{
	double axial_matrix[16] =
	{
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0
	};

	

	vtkSmartPointer<vtkDICOMImageReader> dicomReader = vtkSmartPointer<vtkDICOMImageReader>::New();
	dicomReader->SetDirectoryName("D:\\DicomDataSet\\Sharmila");
	dicomReader->SetFileLowerLeft(1);
	dicomReader->Update();

	// setup direction cosines
	float* orientation;
	orientation = dicomReader->GetImageOrientationPatient();
	

	double normal[3] = { 0, 0, 0 };
	normal[0] = (orientation[1] * orientation[5]) - (orientation[2] * orientation[4]);
	normal[1] = (orientation[0] * orientation[5]) - (orientation[2] * orientation[3]);
	normal[2] = (orientation[0] * orientation[4]) - (orientation[1] * orientation[3]);

	axial_matrix[0] = orientation[0];
	axial_matrix[1] = orientation[1];
	axial_matrix[2] = orientation[2];
	axial_matrix[3] = 0;

	axial_matrix[4] = orientation[3];
	axial_matrix[5] = orientation[4];
	axial_matrix[6] = orientation[5];
	axial_matrix[7] = 0;

	axial_matrix[8] = normal[0];
	axial_matrix[9] = normal[1];
	axial_matrix[10] = normal[2];
	axial_matrix[11] = 0;

	axial_matrix[12] = 0;
	axial_matrix[13] = 0;
	axial_matrix[14] = 0;
	axial_matrix[15] = 1;
	
	vtkSmartPointer<vtkMatrix4x4> direction_cosines = vtkSmartPointer<vtkMatrix4x4>::New();
	direction_cosines->DeepCopy(axial_matrix);
	direction_cosines->Print(cout);
	bool willFlip = false;
	bool flip[3] = { false, false, false };
	for (int j = 0; j < 3; j++){
		if (direction_cosines->GetElement(j, j) < 0)
		{
			flip[j] = true;
			willFlip = true;
		}
		else 
		{
			cout << j << ":" << direction_cosines->GetElement(j, j) << endl;
			flip[j] = false;
		}
	}

	vtkSmartPointer<vtkImageData> dicomData = dicomReader->GetOutput();

	if (willFlip)
	{
		vtkSmartPointer<vtkImageFlip> theFlip = vtkSmartPointer<vtkImageFlip>::New();

		theFlip->SetInputData(dicomData);
		theFlip->FlipAboutOriginOn();
		//	//gotta find the first axis to flip...
		bool keepGoing = false;
		if (flip[0])
		{
			cout << "Flip 1: Axis 0" << endl;
			theFlip->SetFilteredAxis(0);
			keepGoing = flip[1] || flip[2];
		}
		else if (flip[1]){
			cout << "Flip 1: Axis 1" << endl; 
			theFlip->SetFilteredAxis(1);
			keepGoing = flip[2];
		}
		else {
			cout << "Flip 1: Axis 2" << endl;
			theFlip->SetFilteredAxis(2);
		}
		theFlip->Update();

		if (!keepGoing)
				dicomData = theFlip->GetOutput();

		if (keepGoing)
		{
			bool keepGoing2 = false;

			vtkSmartPointer<vtkImageFlip> theFlip2 = vtkSmartPointer<vtkImageFlip>::New();

			theFlip2->SetInputConnection(theFlip->GetOutputPort());
			theFlip2->FlipAboutOriginOn();
			
			if (flip[1]){
				cout << "Flip 2: Axis 1" << endl;
				theFlip2->SetFilteredAxis(1);
				keepGoing2 = flip[2];
			}
			else {
				cout << "Flip 2: Axis 2" << endl;
				theFlip2->SetFilteredAxis(2);
			}

			theFlip2->Update();
			if (!keepGoing2)
				dicomData = theFlip2->GetOutput();
			if (keepGoing2)
			{
				vtkSmartPointer<vtkImageFlip> theFlip3 = vtkSmartPointer<vtkImageFlip>::New();
				theFlip3->SetInputConnection(theFlip2->GetOutputPort());
				theFlip3->FlipAboutOriginOn();
				cout << "Flip 3: Axis 2" << endl;
				theFlip3->SetFilteredAxis(2);

				theFlip3->Update();
				dicomData = theFlip3->GetOutput();

			}
		}
	}

	//have to do this because the flipping above only flips	about 0, 0, 0
	double origin[3] = { 0, 0, 0 };
	dicomData->GetOrigin(origin);

	double spacing[3] = { 0, 0, 0 };
	dicomData->GetSpacing(spacing);
	double bounds[6] = { 0, 0, 0, 0, 0, 0 };
	dicomData->GetBounds(bounds);

	vtkSmartPointer<vtkImageChangeInformation> imageSpacingChange2 = vtkSmartPointer<vtkImageChangeInformation>::New();

	imageSpacingChange2->SetInputData(dicomData);
	imageSpacingChange2->SetOutputSpacing(spacing[0], spacing[1],
		spacing[2]);
	double extent[3] = { 0, 0, 0 };
	for (int j = 0; j < 3; j++){
		extent[j] = bounds[j * 2 + 1] - bounds[j * 2];
	}
	imageSpacingChange2->SetOutputOrigin(
		origin[0] + (flip[0] ? -1 : 0) *extent[0],
		origin[1] + (flip[1] ? -1 : 0) *extent[1],
		origin[2] + (flip[2] ? -1 : 0) *extent[2]);
	imageSpacingChange2->Update();
	dicomData = imageSpacingChange2->GetOutput();
	
	
	vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
	transform->Identity();
	transform->Translate(dicomData->GetCenter());


	vtkSmartPointer<vtkImageReslice> reslice = vtkSmartPointer<vtkImageReslice>::New();
	reslice->SetResliceAxes(transform->GetMatrix());
	
	reslice->SetOutputDimensionality(2);
	reslice->SetInputData(dicomData);
	reslice->Update();
	vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
	lut->SetRange(0, 2000); // image intensity range
	lut->SetValueRange(0.0, 1.0); // from black to white
	lut->SetSaturationRange(0.0, 0.0); // no color saturation
	lut->SetRampToLinear();
	lut->Build();

	//// Map the image through the lookup table
	vtkSmartPointer<vtkImageMapToWindowLevelColors> color = vtkSmartPointer<vtkImageMapToWindowLevelColors>::New();
	color->SetInputData(reslice->GetOutput());
	color->SetOutputFormatToLuminance();
	color->SetWindow(719.0);
	color->SetLevel(8.0);
	/////*color->Update();

	////vtkSmartPointer<vtkImageSliceMapper> image_mapper = vtkSmartPointer<vtkImageSliceMapper>::New();
	////image_mapper->SetInputConnection(color->GetOutputPort());*/

	// Display the image
	vtkSmartPointer<vtkImageActor> actor = vtkSmartPointer<vtkImageActor>::New();
	actor->GetMapper()->SetInputConnection(color->GetOutputPort());

	vtkSmartPointer<vtkOutlineFilter> outline = vtkSmartPointer<vtkOutlineFilter>::New();
	outline->SetInputConnection(reslice->GetOutputPort());
	outline->Update();

	vtkSmartPointer<vtkPolyDataMapper> outline_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	outline_mapper->SetInputData(outline->GetOutput());

	vtkSmartPointer<vtkActor> outline_actor = vtkSmartPointer<vtkActor>::New();
	outline_actor->SetMapper(outline_mapper);
	outline_actor->GetProperty()->SetEdgeVisibility(1);
	outline_actor->GetProperty()->SetEdgeColor(0, 1, 0);

	// Visualize
	vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();

	vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
	renderWindow->AddRenderer(renderer);

	vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	renderWindowInteractor->SetRenderWindow(renderWindow);

	renderer->AddActor(outline_actor);
	renderer->AddActor(actor);
	renderWindow->Render();

	vtkSmartPointer<vtkInteractorStyleImage> imageStyle = vtkSmartPointer<vtkInteractorStyleImage>::New();
	renderWindowInteractor->SetInteractorStyle(imageStyle);
	renderWindow->SetInteractor(renderWindowInteractor);
	renderWindow->Render();

	vtkSmartPointer<vtkImageInteractionCallback> callback = vtkSmartPointer<vtkImageInteractionCallback>::New();
	callback->SetImageReslice(reslice);
	callback->SetInteractor(renderWindowInteractor);

	imageStyle->AddObserver(vtkCommand::MouseMoveEvent, callback);
	imageStyle->AddObserver(vtkCommand::LeftButtonPressEvent, callback);
	imageStyle->AddObserver(vtkCommand::LeftButtonReleaseEvent, callback);

	renderWindowInteractor->Start();

	return 0;
}

