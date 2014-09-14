#pragma once
#include <iostream>

using namespace std;
class MPROrientation
{
public:
	MPROrientation(string top, string left, string bottom, string right);
	~MPROrientation();

	void processOrientations();
	void getRoationXYZ(double outangles[3]);
private:
	void initializePrincipleOrientations();
	int findOrientationType();
	void RotateX(int degree);
	void RotateY(int degree);
	void RotateZ(int degree);

private:
	string m_currentOrientation;
	string m_axialOrientation;
	string m_saggitalOrientation;
	string m_coronalOrientation;

	int m_orientationType;

	int m_angles[3];
};

