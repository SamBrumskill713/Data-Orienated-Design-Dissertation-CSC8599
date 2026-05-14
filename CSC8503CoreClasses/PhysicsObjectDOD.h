#pragma once
#include "Vector.h"
#include "Quaternion.h"
#include "Matrix.h"

using namespace NCL::Maths;

namespace NCL::CSC8503 {

	// Pure data component - mirrors member variables from original class
	struct PhysicsObjectComp {
		Matrix3 inverseInertiaTensor;
		Vector3 linearVelocity;
		Vector3 force;

		Vector3 angularVelocity;
		Vector3 torque;
		Vector3 inverseInertia;
		float inverseMass;
		float elasticity;
		bool isCollided;
		PhysicsObjectComp()
			: linearVelocity(Vector3()),
			force(Vector3()),
			angularVelocity(Vector3()),
			torque(Vector3()),
			inverseInertia(Vector3()),
			inverseMass(1.0f),
			elasticity(0.8f),
			isCollided(false) {
		}
	};

	struct PhysicsObjectSys {
		PhysicsObjectComp data;

		Vector3 GetLinearVelocity() const {
			return data.linearVelocity;
		}

		Vector3 GetAngularVelocity() const {
			return data.angularVelocity;
		}

		Vector3 GetTorque() const {
			return data.torque;
		}

		Vector3 GetForce() const {
			return data.force;
		}

		float GetInverseMass() const {
			return data.inverseMass;
		}

		float GetElasticity() const {
			return data.elasticity;
		}

		bool GetIsCollider() const {
			return data.isCollided;
		}

		Matrix3 GetInertiaTensor() const {
			return data.inverseInertiaTensor;
		}

		PhysicsObjectSys& SetInverseMass(float invMass) {
			data.inverseMass = invMass;
			return *this;
		}

		PhysicsObjectSys& SetLinearVelocity(const Vector3& v) {
			data.linearVelocity = v;
			return *this;
		}

		PhysicsObjectSys& SetAngularVelocity(const Vector3& v) {
			data.angularVelocity = v;
			return *this;
		}

		PhysicsObjectSys& SetElasticity(float e) {
			data.elasticity = e;
			return *this;
		}

		void SetIsCollider(bool colliding) {
			data.isCollided = colliding;
		}

		// Methods - implementation takes transform data as parameter instead of storing reference
		void ApplyAngularImpulse(const Vector3& impulse);
		void ApplyLinearImpulse(const Vector3& impulse);
		void AddForce(const Vector3& addedForce);
		void AddForceAtPosition(const Vector3& addedForce, const Vector3& position, const Vector3& transformPosition);
		void AddTorque(const Vector3& addedTorque);
		void ClearForces();
		void InitCubeInertia(const Vector3& scale);
		void InitSphereInertia(const Vector3& scale);
		void UpdateInertiaTensor(const Quaternion& orientation);
	};
}