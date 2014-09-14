#pragma once

namespace RTViewer
{
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
}