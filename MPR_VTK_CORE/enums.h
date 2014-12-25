#pragma once

namespace RTViewer
{
	enum DVHType{Differential=0, Cumulative=1};
	enum DoseUnit { Gy = 0, cGy = 1, Percent=2 };

	enum Axis{ AxialAxis=0, CoronalAxis=1, SagittalAxis=2};

	static double sagittalElements[16] = {
			 0, 0,-1, 0,
			 1, 0, 0, 0,
			 0,-1, 0, 0,
			 0, 0, 0, 1 };

	static double axialElements[16] = {
			 1, 0, 0, 0,
			 0, 1, 0, 0,
			 0, 0, 1, 0,
			 0, 0, 0, 1 };

	static double coronalElements[16] = {
		   1, 0, 0, 0,
		   0, 0, 1, 0,
		   0,-1, 0, 0,
		   0, 0, 0, 1 };

	
	static double sagittalCosines[9] = {
		0, 0, -1,
		1, 0, 0, 
		0, -1, 0};

	static double axialCosines[9] = {
		1, 0, 0,
		0, 1, 0,
		0, 0, 1 };

	static double coronalCosines[9] = {
		1, 0, 0,
		0, 0, 1,
		0, -1, 0 };
}