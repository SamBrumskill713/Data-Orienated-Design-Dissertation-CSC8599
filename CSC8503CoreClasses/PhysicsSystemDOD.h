#pragma once

#include <vector>
#include "Vector.h"
#include "Quaternion.h"
#include "Matrix.h"
#include "GameWorldDOD.h"
#include "GameObjectDOD.h"
#include "CollisionDetectionDOD.h"

using namespace NCL::Maths;

namespace NCL::CSC8503 {
	struct PhysicsSystemData {
		Vector3 gravity;
		float globalDamping;
		float dTOffset;
		bool applyGravity;
		bool useBroadPhase;
		int numCollisionFrames;

		PhysicsSystemData()
			: gravity(Vector3(0.0f, -9.8f, 0.0f)), globalDamping(0.995f),
			dTOffset(0.0f), applyGravity(false), useBroadPhase(false), numCollisionFrames(5) {
		}
	};

	struct BroadphasePair {
		size_t indexA;
		size_t indexB;

		BroadphasePair() : indexA(0), indexB(0) {}
		BroadphasePair(size_t a, size_t b) : indexA(a), indexB(b) {}

		bool operator==(const BroadphasePair& other) const {
			return (indexA == other.indexA && indexB == other.indexB) ||
				(indexA == other.indexB && indexB == other.indexA);
		}

		bool operator<(const BroadphasePair& other) const {
			if (indexA != other.indexA) return indexA < other.indexA;
			return indexB < other.indexB;
		}
	};

	struct ActiveCollisionDOD {
		size_t entityA;
		size_t entityB;
		int framesLeft;

		ActiveCollisionDOD() : entityA(0), entityB(0), framesLeft(0) {}
		ActiveCollisionDOD(size_t a, size_t b, int frames)
			: entityA(a), entityB(b), framesLeft(frames) {
		}

		bool operator==(const ActiveCollisionDOD& other) const {
			return (entityA == other.entityA && entityB == other.entityB) ||
				(entityA == other.entityB && entityB == other.entityA);
		}
	};

	class PhysicsSystemDOD {
	public:
		PhysicsSystemDOD(GameWorldDOD& world);
		~PhysicsSystemDOD() = default;

		PhysicsSystemData data;

		void SetGravity(const Vector3& g) {
			data.gravity = g;
		}

		void UseGravity(bool state) {
			data.applyGravity = state;
		}

		void SetGlobalDamping(float d) {
			data.globalDamping = d;
		}

		void UseBroadPhase(bool state) {
			data.useBroadPhase = state;
		}

		bool IsBroadPhaseEnabled() const {
			return data.useBroadPhase;
		}

		void Clear();
		void Update(float dt);

	private:
		GameWorldDOD& gameWorld;
		std::vector<ActiveCollisionDOD> activeCollisions;
		std::vector<BroadphasePair> broadphasePairs;

		void IntegrateAccel(float dt);
		void IntegrateVelocity(float dt);
		void ClearForces();
		void DetectCollisions();
		void UpdateCollisionList();

		void BroadPhase();
		void NarrowPhase();
		void BasicCollisionDetection();

		void ImpulseResolveCollision(PhysicsObjectComp& physA, PhysicsObjectComp& physB,
			TransformsComp& transA, TransformsComp& transB,
			const Vector3& normal, const Vector3& localA, const Vector3& localB, float penetration) const;
	};
}