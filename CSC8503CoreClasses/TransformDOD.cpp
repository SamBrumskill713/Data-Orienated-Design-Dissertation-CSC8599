#include "TransformDOD.h"

using namespace NCL::CSC8503;

void TransformsSys::UpdateMatrix() {
	transformData.matrix =
		Matrix::Translation(transformData.position) *
		Quaternion::RotationMatrix<Matrix4>(transformData.orientation) *
		Matrix::Scale(transformData.scale);
}

TransformsComp& TransformsSys::SetPosition(const Vector3& worldPos) {
	transformData.position = worldPos;
	UpdateMatrix();
	return transformData;
}

TransformsComp& TransformsSys::SetScale(const Vector3& worldScale) {
	transformData.scale = worldScale;
	UpdateMatrix();
	return transformData;
}

TransformsComp& TransformsSys::SetOrientation(const Quaternion& newOr) {
	transformData.orientation = newOr;
	UpdateMatrix();
	return transformData;
}