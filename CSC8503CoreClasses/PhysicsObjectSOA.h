#pragma once

#include "Vector.h"
#include "Quaternion.h"
#include "Matrix.h"
#include <vector>

using namespace NCL::Maths;

namespace NCL::CSC8503 {
	struct PhysicsObjectCompSOA {
		std::vector<Matrix3> inverseInertiaTensorSOA;
		std::vector<Vector3> linearVelocitySOA;
		std::vector<Vector3> forceSOA;
		std::vector<Vector3> angularVelocitySOA;
		std::vector<Vector3> torqueSOA;
		std::vector<Vector3> inverseInertiaSOA;
		std::vector<float> inverseMassSOA;
		std::vector<float> elasticitySOA;
		std::vector<float> frictionSOA;
		std::vector<char> isCollidedSOA;
	};

	namespace PhysicsOpsSOA {

		inline int GetCount(const PhysicsObjectCompSOA& bodyData) {
			return bodyData.inverseMassSOA.size();
		}

		inline int AddPhysicsBody(PhysicsObjectCompSOA& bodyData,
			const Vector3& linearVelocity = Vector3(),
			const Vector3& force = Vector3(),
			const Vector3& angularVelocity = Vector3(),
			const Vector3& torque = Vector3(),
			const Vector3& inverseInertia = Vector3(),
			const float& inverseMass = 1.0f,
			const float& elasticity = 0.8f,
			const float& friction = 0.8f,
			const bool& isCollided = false) {
			int index = bodyData.inverseMassSOA.size();
			bodyData.inverseInertiaTensorSOA.push_back(Matrix3());
			bodyData.linearVelocitySOA.push_back(linearVelocity);
			bodyData.forceSOA.push_back(force);
			bodyData.angularVelocitySOA.push_back(angularVelocity);
			bodyData.torqueSOA.push_back(torque);
			bodyData.inverseInertiaSOA.push_back(inverseInertia);
			bodyData.inverseMassSOA.push_back(inverseMass);
			bodyData.elasticitySOA.push_back(elasticity);
			bodyData.frictionSOA.push_back(friction);
			bodyData.isCollidedSOA.push_back(isCollided);
			return index;
		}

		inline void RemovePhysicsBody(PhysicsObjectCompSOA& bodyData, int index) {
			if (index < 0 || index >= (int)bodyData.inverseMassSOA.size()) return;

			int lastIndex = bodyData.inverseMassSOA.size() - 1;
			if (index != lastIndex) {
				bodyData.inverseInertiaTensorSOA[index] = bodyData.inverseInertiaTensorSOA[lastIndex];
				bodyData.linearVelocitySOA[index] = bodyData.linearVelocitySOA[lastIndex];
				bodyData.forceSOA[index] = bodyData.forceSOA[lastIndex];
				bodyData.angularVelocitySOA[index] = bodyData.angularVelocitySOA[lastIndex];
				bodyData.torqueSOA[index] = bodyData.torqueSOA[lastIndex];
				bodyData.inverseInertiaSOA[index] = bodyData.inverseInertiaSOA[lastIndex];
				bodyData.inverseMassSOA[index] = bodyData.inverseMassSOA[lastIndex];
				bodyData.elasticitySOA[index] = bodyData.elasticitySOA[lastIndex];
				bodyData.frictionSOA[index] = bodyData.frictionSOA[lastIndex];
				bodyData.isCollidedSOA[index] = bodyData.isCollidedSOA[lastIndex];
			}
			bodyData.inverseInertiaTensorSOA.pop_back();
			bodyData.linearVelocitySOA.pop_back();
			bodyData.forceSOA.pop_back();
			bodyData.angularVelocitySOA.pop_back();
			bodyData.torqueSOA.pop_back();
			bodyData.inverseInertiaSOA.pop_back();
			bodyData.inverseMassSOA.pop_back();
			bodyData.elasticitySOA.pop_back();
			bodyData.frictionSOA.pop_back();
			bodyData.isCollidedSOA.pop_back();
		}

		inline void UpdateInertiaTensor(PhysicsObjectCompSOA& bodyData, const Quaternion& orientation, int index) {
			Matrix3 invOrientation = Quaternion::RotationMatrix<Matrix3>(orientation.Conjugate());
			Matrix3 orientationMatrix = Quaternion::RotationMatrix<Matrix3>(orientation);
			bodyData.inverseInertiaTensorSOA[index] = orientationMatrix * Matrix::Scale3x3(bodyData.inverseInertiaSOA[index]) * invOrientation;
		}

		inline void InitCubeInertia(PhysicsObjectCompSOA& bodyData, const Vector3& scale, int index) {
			Vector3 fullWidth = scale * 2.0f;
			Vector3 dimSqr = fullWidth * fullWidth;

			bodyData.inverseInertiaSOA[index].x = (12.0f * bodyData.inverseMassSOA[index]) / (dimSqr.y + dimSqr.z);
			bodyData.inverseInertiaSOA[index].y = (12.0f * bodyData.inverseMassSOA[index]) / (dimSqr.x + dimSqr.z);
			bodyData.inverseInertiaSOA[index].z = (12.0f * bodyData.inverseMassSOA[index]) / (dimSqr.x + dimSqr.y);
		}

		inline void ApplyLinearImpulse(PhysicsObjectCompSOA& bodyData, const Vector3& impulse, int index) {
			bodyData.linearVelocitySOA[index] += impulse * bodyData.inverseMassSOA[index];
		}

		inline void ApplyAngularImpulse(PhysicsObjectCompSOA& bodyData, const Vector3& angulerImpulse, int index) {
			bodyData.angularVelocitySOA[index] += bodyData.inverseInertiaTensorSOA[index] * angulerImpulse;
		}

		inline void AddForce(PhysicsObjectCompSOA& bodyData, const Vector3 addedForce, int index) {
			bodyData.forceSOA[index] += addedForce;
		}

		inline void AddTorque(PhysicsObjectCompSOA& bodyData, const Vector3 addedTorque, int index) {
			bodyData.torqueSOA[index] += addedTorque;
		}

		inline void ClearForces(PhysicsObjectCompSOA& bodyData, int index) {
			bodyData.forceSOA[index] = Vector3(0, 0, 0);
			bodyData.torqueSOA[index] = Vector3(0, 0, 0);
		}

		inline void SetInverseMass(PhysicsObjectCompSOA& bodyData, float mass, int index) {
			bodyData.inverseMassSOA[index] = mass;
		}

		inline void ClearAllForces(PhysicsObjectCompSOA& bodyData) {
			int count = GetCount(bodyData);
			for (int i = 0; i < count; ++i) {
				bodyData.forceSOA[i] = Vector3(0, 0, 0);
				bodyData.torqueSOA[i] = Vector3(0, 0, 0);
			}
		}
	}
}