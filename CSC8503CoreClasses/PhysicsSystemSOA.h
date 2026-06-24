#pragma once

#include <vector>
#include "Vector.h"
#include "Quaternion.h"
#include "Matrix.h"
#include "GameWorldSOA.h"
#include "GameObjectSOA.h"
#include "CollisionDetectionSOA.h"

using namespace NCL::Maths;

namespace NCL::CSC8503 {
	// Global physics system configuration (doesn't go in SOA arrays)
	struct PhysicsSystemDataSOA {
		Vector3 gravity;
		float globalDamping;
		float dTOffset;
		bool applyGravity;
		bool useBroadPhase;
		int numCollisionFrames;
		int frameCount;

		PhysicsSystemDataSOA()
			: gravity(Vector3(0.0f, -9.8f, 0.0f)), globalDamping(0.995f),
			dTOffset(0.0f), applyGravity(false), useBroadPhase(false), numCollisionFrames(5) {
		}
	};

	struct BroadphasePairSOA {
		int indexA;
		int indexB;

		BroadphasePairSOA() : indexA(-1), indexB(-1) {}
		BroadphasePairSOA(int a, int b) : indexA(a), indexB(b) {}

		bool operator==(const BroadphasePairSOA& other) const {
			return (indexA == other.indexA && indexB == other.indexB) ||
				(indexA == other.indexB && indexB == other.indexA);
		}

		bool operator<(const BroadphasePairSOA& other) const {
			if (indexA != other.indexA) return indexA < other.indexA;
			return indexB < other.indexB;
		}
	};

	struct ActiveCollisionSOA {
		int entityA;
		int entityB;
		int framesLeft;

		ActiveCollisionSOA() : entityA(-1), entityB(-1), framesLeft(0) {}
		ActiveCollisionSOA(int a, int b, int frames)
			: entityA(a), entityB(b), framesLeft(frames) {
		}

		bool operator==(const ActiveCollisionSOA& other) const {
			return (entityA == other.entityA && entityB == other.entityB) ||
				(entityA == other.entityB && entityB == other.entityA);
		}
	};

	class PhysicsSystemSOA {
	public:
		PhysicsSystemSOA(GameWorldSOA& world);
		~PhysicsSystemSOA() = default;

		PhysicsSystemDataSOA data;

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
		GameWorldSOA& gameWorld;
		std::vector<ActiveCollisionSOA> activeCollisions;
		std::vector<BroadphasePairSOA> broadphasePairs;
		std::vector<int> cachedDynamicObjects;
		QuadTreeSOA<int> quadTree;  
		bool quadTreeDirty = true;

		void IntegrateAccel(float dt, int count);
		void IntegrateVelocity(float dt, int count);
		void ClearForces();
		void UpdateCollisionList(int count);

		void BroadPhase(int count);
		void NarrowPhase(int count);
		void BasicCollisionDetection(int count);

		void ImpulseResolveCollision(int indexA, int indexB, const Vector3& normal,
			const Vector3& localA, const Vector3& localB, float penetration);
	};
}