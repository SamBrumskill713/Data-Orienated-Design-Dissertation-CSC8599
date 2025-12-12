#include "OrientationConstraint.h"
#include "GameObject.h"
#include "PhysicsObject.h"
using namespace NCL;
using namespace Maths;
using namespace CSC8503;

OrientationConstraint::OrientationConstraint(GameObject* a, GameObject* b, const Vector3& allowedAxis)
{
	objectA = a;
	objectB = b;
	hingeAxis = Vector::Normalise(allowedAxis);
}

void OrientationConstraint::UpdateConstraint(float dt) {
	Vector3 wA = objectA->GetPhysicsObject()->GetAngularVelocity();
	Vector3 wB = objectB->GetPhysicsObject()->GetAngularVelocity();

	auto projectYaw = [&](const Vector3& w) {
		return hingeAxis * Vector::Dot(w, hingeAxis);
		};

	Vector3 wAYaw = projectYaw(wA);
	Vector3 wBYaw = projectYaw(wB);

	Vector3 wAPitchRoll = wA - wAYaw;
	Vector3 wBPitchRoll = wB - wBYaw;

	float totalInverseMass = objectA->GetPhysicsObject()->GetInverseMass();
	if (totalInverseMass > 0) {
		const float bias = 0.2f;
		Vector3 cancelA = -wAPitchRoll * bias;
		Vector3 cancelB = -wBPitchRoll * bias;

		Vector3 impulseA = cancelA / std::max(objectA->GetPhysicsObject()->GetInverseMass(), 1e-6f);
		Vector3 impulseB = cancelB / std::max(objectB->GetPhysicsObject()->GetInverseMass(), 1e-6f);

		objectA->GetPhysicsObject()->ApplyAngularImpulse(impulseA);
		objectB->GetPhysicsObject()->ApplyAngularImpulse(impulseB);
	}
}