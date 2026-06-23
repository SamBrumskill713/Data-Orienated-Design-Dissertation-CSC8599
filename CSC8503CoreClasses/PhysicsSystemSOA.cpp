#include "PhysicsSystemSOA.h"
#include "CollisionDetectionSOA.h"
#include "TransformSOA.h"
#include <immintrin.h>
#include <algorithm>

using namespace NCL;
using namespace NCL::CSC8503;

const int IDEAL_HZ = 60;
const float IDEAL_DT = 1.0f / IDEAL_HZ;

PhysicsSystemSOA::PhysicsSystemSOA(GameWorldSOA& world)
	: gameWorld(world) {
}

void PhysicsSystemSOA::Clear() {
	activeCollisions.clear();
	broadphasePairs.clear();
	data.dTOffset = 0.0f;
}

void PhysicsSystemSOA::Update(float dt) {
	data.dTOffset += dt;

	float realDT = IDEAL_DT;

	while (data.dTOffset > realDT) {
		int count = gameWorld.GetObjectCount();
		IntegrateAccel(realDT, count);

		if (data.useBroadPhase) {
			BroadPhase(count);
			NarrowPhase();
		}
		else {
			BasicCollisionDetection(count);
		}

		IntegrateVelocity(realDT, count);
		ClearForces();
		UpdateCollisionList(count);

		data.dTOffset -= realDT;
	}
}

void PhysicsSystemSOA::IntegrateAccel(float dt, int count) {
	auto& objects = gameWorld.gameObjects;

	for (int i = 0; i < count; ++i) {
		if (!objects.isActive[i]) {
			continue;
		}

		float inverseMass = objects.physics.inverseMassSOA[i];
		if (inverseMass == 0.0f) {
			continue;
		}

		Vector3 accel = objects.physics.forceSOA[i] * inverseMass;
		if (data.applyGravity) {
			accel += data.gravity;
		}
		objects.physics.linearVelocitySOA[i] += accel * dt;

		Vector3 angAccel = objects.physics.inverseInertiaTensorSOA[i] * objects.physics.torqueSOA[i];
		objects.physics.angularVelocitySOA[i] += angAccel * dt;

		PhysicsOpsSOA::UpdateInertiaTensor(objects.physics, objects.transforms.orientations[i], i);
	}
}

void PhysicsSystemSOA::IntegrateVelocity(float dt, int count) {
	auto& objects = gameWorld.gameObjects;
	const float LINEAR_DAMPING = 0.4f * dt;
	const float ANGULAR_DAMPING = 0.4f * dt;

	for (int i = 0; i < count; ++i) {
		if (!objects.isActive[i]) {
			continue;
		}

		if (objects.physics.inverseMassSOA[i] == 0.0f) {
			continue;
		}

		objects.transforms.positions[i] += objects.physics.linearVelocitySOA[i] * dt;
		objects.physics.linearVelocitySOA[i] *= (1.0f - LINEAR_DAMPING);

		objects.transforms.orientations[i] = objects.transforms.orientations[i] +
			(Quaternion(objects.physics.angularVelocitySOA[i] * dt * 0.5f, 0.0f) * objects.transforms.orientations[i]);
		objects.transforms.orientations[i].Normalise();

		objects.physics.angularVelocitySOA[i] *= (1.0f - ANGULAR_DAMPING);

		TransformOpsSOA::UpdateMatrixSOA(objects.transforms, i);
	}
}

void PhysicsSystemSOA::ClearForces() {
	auto& objects = gameWorld.gameObjects;
	PhysicsOpsSOA::ClearAllForces(objects.physics);
}

// Pre-filter active dynamic objects at initialization
void PhysicsSystemSOA::BroadPhase(int count) {
	broadphasePairs.clear();

	auto& objects = gameWorld.gameObjects;
	auto& isActive = objects.isActive;
	auto& inverseMass = objects.physics.inverseMassSOA;
	auto& positions = objects.transforms.positions;
	auto& halfSizes = objects.collision.AABBDataSOA.halfSizesSOA;

	// ==== SIMD-friendly pre-filtering pass ====
	// Build indices of dynamic objects to avoid repeated branching
	cachedDynamicObjects.clear();
	cachedDynamicObjects.reserve(count);

	// Process in 8-element chunks with SIMD hints
	int i = 0;
#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

	// Vectorizable loop with direct array access
	for (; i < count; ++i) {
		// These compiles to simple memory loads without overhead
		if (isActive[i] && inverseMass[i] != 0.0f) {
			cachedDynamicObjects.push_back(i);
		}
	}

	// Separate quadtree for only dynamic objects
	QuadTreeSOA<int> quadTree(Vector2(1000.0f, 1000.0f), 6, 10);

	// Batch insert dynamic objects into quadtree
	const int* dynIndices = cachedDynamicObjects.data();
	const int dynCount = (int)cachedDynamicObjects.size();

	for (int i = 0; i < dynCount; ++i) {
		int idx = dynIndices[i];
		Vector3 halfSize = halfSizes[idx];
		quadTree.Insert(idx, positions[idx], halfSize * 2.0f);
	}

	// Pre-allocate with better estimate
	int estimatedPairs = dynCount * 10;
	broadphasePairs.reserve(estimatedPairs);

	// ==== Quadtree pair generation ====
	quadTree.OperateOnContents(
		[&](std::vector<QuadTreeEntrySOA<int>>& contents) {
			const size_t sz = contents.size();
			// SIMD loop: compare pairs in same quadtree node
			for (size_t i = 0; i < sz; ++i) {
				for (size_t j = i + 1; j < sz; ++j) {
					int idxA = contents[i].object;
					int idxB = contents[j].object;
					if (idxA > idxB) std::swap(idxA, idxB);
					broadphasePairs.push_back(BroadphasePairSOA(idxA, idxB));
				}
			}
		}
	);

	// ==== Static-vs-dynamic check (SIMD-friendly) ====
	// Use cached dynamic indices to avoid quadratic scan
	std::vector<int> staticObjects;
	staticObjects.reserve(count - dynCount);

	for (int i = 0; i < count; ++i) {
		if (isActive[i] && inverseMass[i] == 0.0f) {
			staticObjects.push_back(i);
		}
	}

	// Batch pair generation with tighter loop
	const int* staticIndices = staticObjects.data();
	const int staticCount = (int)staticObjects.size();

	for (int i = 0; i < staticCount; ++i) {
		int staticIdx = staticIndices[i];

		// Inner loop can be better optimized by compiler
		for (int j = 0; j < dynCount; ++j) {
			int dynIdx = dynIndices[j];
			int idxA = staticIdx;
			int idxB = dynIdx;
			if (idxA > idxB) std::swap(idxA, idxB);

			broadphasePairs.push_back(BroadphasePairSOA(idxA, idxB));
		}
	}

	// ==== Deduplication (already mostly sorted) ====
	std::sort(broadphasePairs.begin(), broadphasePairs.end());
	auto last = std::unique(broadphasePairs.begin(), broadphasePairs.end());
	broadphasePairs.erase(last, broadphasePairs.end());
}

void PhysicsSystemSOA::NarrowPhase() {
	CollisionInfoSOA collisionInfo;
	int count = gameWorld.GetObjectCount();

	for (const auto& pair : broadphasePairs) {
		if (pair.indexA >= count || pair.indexB >= count) {
			continue;
		}

		if (!gameWorld.gameObjects.isActive[pair.indexA] || !gameWorld.gameObjects.isActive[pair.indexB]) {
			continue;
		}

		if (gameWorld.gameObjects.physics.inverseMassSOA[pair.indexA] == 0.0f &&
			gameWorld.gameObjects.physics.inverseMassSOA[pair.indexB] == 0.0f) {
			continue;
		}

		if (CollisionDetectionSOA::ObjectIntersection(gameWorld.gameObjects, pair.indexA, pair.indexB, collisionInfo)) {
			gameWorld.gameObjects.isCollided[pair.indexA] = true;
			gameWorld.gameObjects.isCollided[pair.indexB] = true;

			ImpulseResolveCollision(pair.indexA, pair.indexB, collisionInfo.normal,
				collisionInfo.localA, collisionInfo.localB, collisionInfo.penetration);

			activeCollisions.push_back(
				ActiveCollisionSOA(pair.indexA, pair.indexB, data.numCollisionFrames)
			);
		}
	}
}

void PhysicsSystemSOA::BasicCollisionDetection(int count) {
	CollisionInfoSOA collisionInfo;

	for (int i = 0; i < count; ++i) {
		if (!gameWorld.gameObjects.isActive[i]) {
			continue;
		}

		for (int j = i + 1; j < count; ++j) {
			if (!gameWorld.gameObjects.isActive[j]) {
				continue;
			}

			if (gameWorld.gameObjects.physics.inverseMassSOA[i] == 0.0f &&
				gameWorld.gameObjects.physics.inverseMassSOA[j] == 0.0f) {
				continue;
			}

			if (CollisionDetectionSOA::ObjectIntersection(gameWorld.gameObjects, i, j, collisionInfo)) {
				gameWorld.gameObjects.isCollided[i] = true;
				gameWorld.gameObjects.isCollided[j] = true;

				ImpulseResolveCollision(i, j, collisionInfo.normal, collisionInfo.localA, collisionInfo.localB, collisionInfo.penetration);

				activeCollisions.push_back(
					ActiveCollisionSOA(i, j, data.numCollisionFrames)
				);
			}
		}
	}
}

void PhysicsSystemSOA::UpdateCollisionList(int count) {
	auto& objects = gameWorld.gameObjects;

	for (auto it = activeCollisions.begin(); it != activeCollisions.end(); ) {
		it->framesLeft--;

		if (it->framesLeft < 0) {
			it = activeCollisions.erase(it);
		}
		else {
			++it;
		}
	}

	for (int i = 0; i < count; ++i) {
		objects.isCollided[i] = false;
	}
}

void PhysicsSystemSOA::ImpulseResolveCollision(int indexA, int indexB, const Vector3& normal,
	const Vector3& localA, const Vector3& localB, float penetration) {
	auto& physicsA = gameWorld.gameObjects.physics;
	auto& physicsB = gameWorld.gameObjects.physics;
	auto& transformsA = gameWorld.gameObjects.transforms;
	auto& transformsB = gameWorld.gameObjects.transforms;

	float totalMass = physicsA.inverseMassSOA[indexA] + physicsB.inverseMassSOA[indexB];
	if (totalMass == 0.0f) {
		return;
	}

	transformsA.positions[indexA] -= normal * penetration * (physicsA.inverseMassSOA[indexA] / totalMass);
	transformsB.positions[indexB] += normal * penetration * (physicsB.inverseMassSOA[indexB] / totalMass);

	Vector3 angVelA = Vector::Cross(physicsA.angularVelocitySOA[indexA], localA);
	Vector3 angVelB = Vector::Cross(physicsB.angularVelocitySOA[indexB], localB);

	Vector3 fullVelA = physicsA.linearVelocitySOA[indexA] + angVelA;
	Vector3 fullVelB = physicsB.linearVelocitySOA[indexB] + angVelB;

	Vector3 contactVel = fullVelB - fullVelA;
	float impulseForce = Vector::Dot(contactVel, normal);

	if (impulseForce >= 0.0f) {
		return;
	}

	Vector3 inertiaA = Vector::Cross(physicsA.inverseInertiaTensorSOA[indexA] *
		Vector::Cross(localA, normal), localA);
	Vector3 inertiaB = Vector::Cross(physicsB.inverseInertiaTensorSOA[indexB] *
		Vector::Cross(localB, normal), localB);

	float angularEffect = Vector::Dot(inertiaA + inertiaB, normal);

	const float restitution = 0.66f;
	float j = (-(1.0f + restitution) * impulseForce) / (totalMass + angularEffect);

	Vector3 fullImpulse = normal * j;

	PhysicsOpsSOA::ApplyLinearImpulse(physicsA, -fullImpulse, indexA);
	PhysicsOpsSOA::ApplyLinearImpulse(physicsB, fullImpulse, indexB);

	if (physicsA.inverseMassSOA[indexA] > 0.0f) {
		PhysicsOpsSOA::ApplyAngularImpulse(physicsA, Vector::Cross(localA, -fullImpulse), indexA);
	}
	if (physicsB.inverseMassSOA[indexB] > 0.0f) {
		PhysicsOpsSOA::ApplyAngularImpulse(physicsB, Vector::Cross(localB, fullImpulse), indexB);
	}

	TransformOpsSOA::UpdateMatrixSOA(transformsA, indexA);
	TransformOpsSOA::UpdateMatrixSOA(transformsB, indexB);
}