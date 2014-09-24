#pragma once
#include <iostream>
#include <vector>
#include "vtkSmartPointer.h"
#include "vtkImageData.h"
#include "enums.h"
#include "rad_util.h"
using namespace std;
namespace RTViewer
{
	struct MPRData;
	class MPR
	{
	public:
		MPR(void);
		~MPR(void);

		void initFromDir(string dirPath);
		void initFromDir1(vector<string> dicomFiles);
		void initFromImage(vtkSmartPointer<vtkImageData> image);
		image GetOutputImage(Axis axis);
		void Scroll(Axis axis, int delta);
		void Scroll2(Axis axis, float newPosition);
		int GetNumberOfImages(Axis axis);
		int GetCurrentImageIndex(Axis axis);
		double GetCurrentImagePosition(Axis axis);
		void GetCurrentSlicerPositionRelativeToIndex(Axis axis, int& xPos, int& yPos);
		void GetOutputImageDisplayDimensions(Axis axis, int& width, int& height);
		string GetOrientationMarkerLeft(Axis axis);
		string GetOrientationMarkerRight(Axis axis);
		string GetOrientationMarkerTop(Axis axis);
		string GetOrientationMarkerBottom(Axis axis);
		void GetXYZPixelSpacing(int axis, double* spacing);

		double GetPixelSpacing(int axis);
		long int GetPixelIntensity(Axis axis, int x_pos, int y_pos);

		void RotateAxesAlongPlane(int axis, int angle);

	private:
		MPRData* d;
	};
}