#pragma once
#include "vtkObject.h"
#include "vtkSmartPointer.h"
#include "vtkTransform.h"

namespace RTViewer
{
class MPRTransform :
	public vtkObject
{
public:
	static MPRTransform* New();
	void Identity() {
		Reset();
	}

	void Translate(double x, double y, double z) {
		m_translation->Translate(x, y, z);
		rotated = false;
		Modified();
	}

	void Translate(double t[]) {
		m_translation->Translate(t[0], t[1], t[2]);
		rotated = false;
		Modified();
	}

	void RotateX(double x) {
		m_rotation->RotateX(x);
		m_rotationVec[0] += x;
		rotated = true;
		Modified();
	}

	void RotateY(double y) {
		m_rotation->RotateY(y);
		m_rotationVec[1] += y;
		rotated = true;
		Modified();
	}

	void RotateZ(double z) {
		m_rotation->RotateZ(z);
		m_rotationVec[2] += z;
		rotated = true;
		Modified();
	}

	void ResetRotations() {

		// FIXME : BUG check should it be setmatrix?
		m_rotation->Identity();//rotation->SetMatrix( );
		for (int i = 0; i<3; i++)
			m_rotationVec[i] = 0;
		rotated = true;
		Modified();
	}

	void Reset() {
		m_rotation->Identity();
		m_translation->Identity();
		for (int i = 0; i<3; i++)
			m_rotationVec[i] = 0;
		rotated = true;
		Modified();
	}

	vtkTransform* transform() const {
		return m_rotation;
	}

	vtkTransform* translationTransform() const {
		return m_translation;
	}

	void TransformPoint(double pos1[], double pos2[]) const {
		m_rotation->TransformPoint(pos1, pos2);
	}

	const double* GetRotation() const { return m_rotationVec; }

	bool wasRotated() const { return rotated; }

private:
	MPRTransform() {
		m_translation = vtkTransform::New();

		m_rotation = vtkTransform::New();
		m_rotation->SetInput(m_translation);

		for (int i = 0; i<3; i++)
			m_rotationVec[i] = 0;

		rotated = false;
	}

	~MPRTransform() {
		m_translation->Delete();
		m_rotation->Delete();
	}

	vtkTransform* m_translation;
	vtkTransform* m_rotation;
	double m_rotationVec[3];
	bool rotated;
};
}