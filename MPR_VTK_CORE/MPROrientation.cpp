#include "MPROrientation.h"

#include <vector>

MPROrientation::MPROrientation(string top, string left, string bottom, string right)
{
	this->m_currentOrientation.append(top);
	this->m_currentOrientation.append(left);
	this->m_currentOrientation.append(bottom);
	this->m_currentOrientation.append(right);

	initializePrincipleOrientations();

}


MPROrientation::~MPROrientation()
{
}

void MPROrientation::initializePrincipleOrientations()
{
	this->m_axialOrientation = "ALPR";

	this->m_saggitalOrientation = "SPIA";
	//slir.
	this->m_coronalOrientation = "SLIR";

	m_orientationType = findOrientationType();

	for (int i = 0; i<3; ++i)
		this->m_angles[i] = 0;
}

int MPROrientation::findOrientationType()
{

	int i = 0;
	for (i = 0; i<4 && this->m_axialOrientation.find(this->m_currentOrientation[i]) != std::string::npos; ++i);

	if (i == 4) return 0; //Axial.

	for (i = 0; i<4 && this->m_saggitalOrientation.find(this->m_currentOrientation[i]) != std::string::npos; ++i);
	if (i == 4) return 1; //Sag

	for (i = 0; i<4 && this->m_coronalOrientation.find(this->m_currentOrientation[i]) != std::string::npos; ++i);
	if (i == 4) return 2;

	return -1;

}

void MPROrientation::processOrientations()
{
	//find the orientation.
	m_orientationType = findOrientationType();

	vector<char> desriedOrient;
	switch (m_orientationType)
	{
		case 0:
		{
			
			desriedOrient.push_back(m_axialOrientation.at(0));
			desriedOrient.push_back(m_axialOrientation.at(1));
		}
			break;
		case 1:
		{
			desriedOrient.push_back(m_saggitalOrientation.at(0));
			desriedOrient.push_back(m_saggitalOrientation.at(1));
		}
			break;
		case 2:
		{
			desriedOrient.push_back(m_coronalOrientation.at(0));
			desriedOrient.push_back(m_coronalOrientation.at(1));
		}
			break;
	}

	for (int i = 0; i < desriedOrient.size(); i++)
	{
		int di = i;
		int ci = m_currentOrientation.find(desriedOrient.at(i));

		int diff = (ci - di);
		if (diff == 0)
			continue;

		if (diff % 2 == 1)
		{

			RotateZ(diff * 90);
			std::cout << "RoatateZ : " << diff * 90 << std::endl;
			this->m_angles[2] = diff * 90;
		}

		else if (diff % 2 == 0)
		{
			if (di == 0)
			{
				RotateX(180);
				std::cout << "RotateX : 180" << std::endl;
				this->m_angles[0] = 180;
			}
			else
			{
				std::cout << "RotateY : 180" << std::endl;
				this->m_angles[1] = 180;
				RotateY(180);
			}
		}
	}
	

}

void MPROrientation::getRoationXYZ(double outangles[3])
{
	for (int i = 0; i<3; ++i)
		outangles[i] = this->m_angles[i];
}

void MPROrientation::RotateY(int degree)
{
	
	char temp = m_currentOrientation[1];
	m_currentOrientation[1] = m_currentOrientation[3];
	m_currentOrientation[3] = temp;

	//printCurrentOrient();
}

void MPROrientation::RotateX(int degree)
{
	
	char temp = m_currentOrientation[0];
	m_currentOrientation[0] = m_currentOrientation[2];
	m_currentOrientation[2] = temp;

	//printCurrentOrient();
}

void MPROrientation::RotateZ(int degree)
{

	if (degree > 0)
	{
		for (int count = 0; count <(degree / 90); ++count)
		{
			char temp = m_currentOrientation[0];
			for (int i = 0; i<3; ++i)
			{
				m_currentOrientation[i] = m_currentOrientation[i + 1];
			}
			m_currentOrientation[3] = temp;
		}
	}
	else
	{
		degree = 0 - degree;  //negating the degree.
		for (int count = 0; count <(degree / 90); ++count)
		{
			char temp = m_currentOrientation[3];
			for (int i = 3; i>0; --i)
			{
				m_currentOrientation[i] = m_currentOrientation[i - 1];
			}
			m_currentOrientation[0] = temp;
		}

	}
	//std::cout<<"After rotationZ : "<<degree<<std::endl;
	//printCurrentOrient();


}
