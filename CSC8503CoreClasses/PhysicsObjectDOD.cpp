#include "PhysicsObjectDOD.h"

using namespace NCL;
using namespace NCL::Maths;
using namespace NCL::CSC8503;

void PhysicsObjectSys::ApplyAngularImpulse(const Vector3& impulse) {
	data.angularVelocity += data.inverseInertiaTensor * impulse;
}

void PhysicsObjectSys::ApplyLinearImpulse(const Vector3& impulse) {
	data.linearVelocity += impulse * data.inverseMass;
}

void PhysicsObjectSys::AddForce(const Vector3& addedForce) {
	data.force += addedForce;
}

void PhysicsObjectSys::AddForceAtPosition(const Vector3& addedForce, const Vector3& position, const Vector3& transformPosition) {
	Vector3 localPos = position - transformPosition;

	data.force += addedForce;
	data.torque += Vector::Cross(localPos, addedForce);
}

void PhysicsObjectSys::AddTorque(const Vector3& addedTorque) {
	data.torque += addedTorque;
}

void PhysicsObjectSys::ClearForces() {
	data.force = Vector3();
	data.torque = Vector3();
}

void PhysicsObjectSys::InitCubeInertia(const Vector3& scale) {
	Vector3 dimensions = scale;
	Vector3 fullWidth = dimensions * 2.0f;
	Vector3 dimsSqr = fullWidth * fullWidth;

	data.inverseInertia.x = (12.0f * data.inverseMass) / (dimsSqr.y + dimsSqr.z);
	data.inverseInertia.y = (12.0f * data.inverseMass) / (dimsSqr.x + dimsSqr.z);
	data.inverseInertia.z = (12.0f * data.inverseMass) / (dimsSqr.x + dimsSqr.y);
}

void PhysicsObjectSys::InitSphereInertia(const Vector3& scale) {
	float radius = Vector::GetMaxElement(scale);
	float i = 2.5f * data.inverseMass / (radius * radius);

	data.inverseInertia = Vector3(i, i, i);
}

void PhysicsObjectSys::UpdateInertiaTensor(const Quaternion& orientation) {
	Matrix3 invOrientation = Quaternion::RotationMatrix<Matrix3>(orientation.Conjugate());
	Matrix3 orientationMat = Quaternion::RotationMatrix<Matrix3>(orientation);

	data.inverseInertiaTensor = orientationMat * Matrix::Scale3x3(data.inverseInertia) * invOrientation;
}