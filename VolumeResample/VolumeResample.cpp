// VolumeResample.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "vtkDICOMImageReader.h"
#include "vtkImageData.h"
#include "vtkImageResample.h"
#include "vtkSmartPointer.h"

class DICOMReader
{
public: 
	DICOMReader()
	{
		this->reader = vtkDICOMImageReader::New();
		this->reader->SetDirectoryName("D:\\Dicomdataset\\HCG-Dicoms\\DicompylerDataset\\CT");
		this->reader->Update();
		this->reader->Print(cout);
		cout << "-------------------------------" << endl;
		this->volume = reader->GetOutput();
		this->volume->Print(cout);
		cout << "-------------------------------" << endl;
	}

	vtkImageData* GetVolume(){
		return this->volume;
	}
private:
	vtkDICOMImageReader* reader;
	vtkImageData* volume;
};
int _tmain(int argc, _TCHAR* argv[])
{
	
	DICOMReader* dr = new DICOMReader();

	vtkImageResample* resample = vtkImageResample::New();
	resample->SetInputData(dr->GetVolume());
	resample->SetDimensionality(3);
	resample->SetAxisOutputSpacing(0, 1);
	resample->SetAxisOutputSpacing(1, 1);
	resample->SetAxisOutputSpacing(2, 1);
	resample->SetInterpolationModeToCubic();
	resample->Update();
	cout << "*************************" << endl;
	vtkImageData* resampled_volume = resample->GetOutput();
	resampled_volume->Print(cout);
	cout << "*************************" << endl;

	return 0;
}

