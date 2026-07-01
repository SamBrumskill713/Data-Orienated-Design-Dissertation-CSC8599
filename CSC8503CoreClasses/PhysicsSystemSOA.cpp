#include "PhysicsSystemSOA.h"
#include "CollisionDetectionSOA.h"
#include "TransformSOA.h"
#include <immintrin.h>
#include <algorithm>
#include <chrono>

using namespace NCL;
using namespace NCL::CSC8503;
using namespace std::chrono;

const int IDEAL_HZ = 60;
const float IDEAL_DT = 1.0f / IDEAL_HZ;

PhysicsSystemSOA::PhysicsSystemSOA(GameWorldSOA& world)
	: gameWorld(world), quadTree(Vector2(1000.0f, 1000.0f), 6, 10) {
}

void PhysicsSystemSOA::Clear() {
	activeCollisions.clear();
	broadphasePairs.clear();
	data.dTOffset = 0.0f;
	quadTreeDirty = true;
}

void PhysicsSystemSOA::Update(float dt) {
	data.dTOffset += dt;
	float realDT = IDEAL_DT;
	int count = gameWorld.GetObjectCount();

	while (data.dTOffset > realDT) {
		auto t0 = std::chrono::high_resolution_clock::now();
		IntegrateAccel(realDT, count);

		auto t1 = std::chrono::high_resolution_clock::now();
		BroadPhase(count);

		auto t2 = std::chrono::high_resolution_clock::now();
		NarrowPhase(count);

		auto t3 = std::chrono::high_resolution_clock::now();
		IntegrateVelocity(realDT, count);

		auto t4 = std::chrono::high_resolution_clock::now();
		ClearForces();

		auto t5 = std::chrono::high_resolution_clock::now();
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

	quadTreeDirty = true;
}

void PhysicsSystemSOA::ClearForces() {
	auto& objects = gameWorld.gameObjects;
	PhysicsOpsSOA::ClearAllForces(objects.physics);
}

void PhysicsSystemSOA::BroadPhase(int count) {
	broadphasePairs.clear();

	auto& objects = gameWorld.gameObjects;
	auto& isActive = objects.isActive;
	auto& inverseMass = objects.physics.inverseMassSOA;
	auto& positions = objects.transforms.positions;
	auto& halfSizes = objects.collision.AABBDataSOA.halfSizesSOA;

	cachedDynamicObjects.clear();
	cachedDynamicObjects.reserve(count);

	for (int i = 0; i < count; ++i) {
		if (isActive[i] && inverseMass[i] != 0.0f) {
			cachedDynamicObjects.push_back(i);
		}
	}

	if (quadTreeDirty) {
		quadTree.Clear(); 

		const int* dynIndices = cachedDynamicObjects.data();
		const int dynCount = (int)cachedDynamicObjects.size();

		for (int i = 0; i < dynCount; ++i) {
			int idx = dynIndices[i];
			Vector3 halfSize = halfSizes[idx];
			quadTree.Insert(idx, positions[idx], halfSize * 2.0f);
		}

		quadTreeDirty = false;  
	}

	int estimatedPairs = (int)cachedDynamicObjects.size() * 10;
	broadphasePairs.reserve(estimatedPairs);

	quadTree.OperateOnContents(
		[&](std::vector<QuadTreeEntrySOA<int>>& contents) {
			const size_t sz = contents.size();
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

	std::vector<int> staticObjects;
	staticObjects.reserve(count - (int)cachedDynamicObjects.size());

	for (int i = 0; i < count; ++i) {
		if (isActive[i] && inverseMass[i] == 0.0f) {
			staticObjects.push_back(i);
		}
	}

	const int* staticIndices = staticObjects.data();
	const int staticCount = (int)staticObjects.size();
	const int* dynIndices = cachedDynamicObjects.data();
	const int dynCount = (int)cachedDynamicObjects.size();

	for (int i = 0; i < staticCount; ++i) {
		int staticIdx = staticIndices[i];
		Vector3 staticPos = positions[staticIdx];
		Vector3 staticSize = halfSizes[staticIdx] * 2.0f;

		for (int j = 0; j < dynCount; ++j) {
			int dynIdx = dynIndices[j];
			Vector3 dynPos = positions[dynIdx];
			Vector3 dynSize = halfSizes[dynIdx] * 2.0f;

			if (std::abs(staticPos.x - dynPos.x) < (staticSize.x + dynSize.x) / 2.0f + 5.0f &&
				std::abs(staticPos.y - dynPos.y) < (staticSize.y + dynSize.y) / 2.0f + 5.0f &&
				std::abs(staticPos.z - dynPos.z) < (staticSize.z + dynSize.z) / 2.0f + 5.0f) {

				int idxA = staticIdx;
				int idxB = dynIdx;
				if (idxA > idxB) std::swap(idxA, idxB);
				broadphasePairs.push_back(BroadphasePairSOA(idxA, idxB));
			}
		}
	}
}

void PhysicsSystemSOA::NarrowPhase(int count) {
	CollisionInfoSOA collisionInfo;

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
					ActiveCollisionSOA(i, j, data.numCollisionFrames));
			}
		}
	}
}

void PhysicsSystemSOA::UpdateCollisionList(int count)
{
	auto& objects = gameWorld.gameObjects;

	for (auto& c : activeCollisions) {
		--c.framesLeft;
	}

	std::erase_if(activeCollisions,
		[](const ActiveCollisionSOA& c) {
			return c.framesLeft < 0;
		});

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