#pragma once

#include "Vector.h"
#include "Quaternion.h"
#include "Matrix.h"

using namespace NCL::Maths;

namespace NCL::CSC8503 {

	struct PhysicsObjectComp {
		Matrix3 inverseInertiaTensor;
		Vector3 linearVelocity;
		Vector3 force;

		Vector3 angularVelocity;
		Vector3 torque;
		Vector3 inverseInertia;
		float inverseMass;
		float elasticity;
		float friction;
		bool isCollided;

		PhysicsObjectComp()
			: linearVelocity(Vector3()),
			force(Vector3()),
			angularVelocity(Vector3()),
			torque(Vector3()),
			inverseInertia(Vector3()),
			inverseMass(1.0f),
			elasticity(0.8f),
			friction(0.8f),
			isCollided(false) {
		}
	};

	namespace PhysicsOps {

		inline void UpdateInertiaTensor(PhysicsObjectComp& physics, const Quaternion& orientation) {
			Matrix3 invOrientation = Quaternion::RotationMatrix<Matrix3>(orientation.Conjugate());
			Matrix3 orientationMatrix = Quaternion::RotationMatrix<Matrix3>(orientation);
			physics.inverseInertiaTensor = orientationMatrix * Matrix::Scale3x3(physics.inverseInertia) * invOrientation;
		}

		inline void InitCubeInertia(PhysicsObjectComp& physics, const Vector3& scale) {
			Vector3 fullWidth = scale * 2.0f;
			Vector3 dimsSqr = fullWidth * fullWidth;

			physics.inverseInertia.x = (12.0f * physics.inverseMass) / (dimsSqr.y + dimsSqr.z);
			physics.inverseInertia.y = (12.0f * physics.inverseMass) / (dimsSqr.x + dimsSqr.z);
			physics.inverseInertia.z = (12.0f * physics.inverseMass) / (dimsSqr.x + dimsSqr.y);
		}

		inline void InitSphereInertia(PhysicsObjectComp& physics, const Vector3& scale) {
			float radius = Vector::GetMaxElement(scale);
			float i = 2.5f * physics.inverseMass / (radius * radius);
			physics.inverseInertia = Vector3(i, i, i);
		}

		inline void ApplyLinearImpulse(PhysicsObjectComp& physics, const Vector3& impulse) {
			physics.linearVelocity += impulse * physics.inverseMass;
		}

		inline void ApplyAngularImpulse(PhysicsObjectComp& physics, const Vector3& impulse) {
			physics.angularVelocity += physics.inverseInertiaTensor * impulse;
		}

		inline void AddForce(PhysicsObjectComp& physics, const Vector3& addedForce) {
			physics.force += addedForce;
		}

		inline void AddTorque(PhysicsObjectComp& physics, const Vector3& addedTorque) {
			physics.torque += addedTorque;
		}

		inline void ClearForces(PhysicsObjectComp& physics) {
			physics.force = Vector3(0, 0, 0);
			physics.torque = Vector3(0, 0, 0);
		}
	}
}