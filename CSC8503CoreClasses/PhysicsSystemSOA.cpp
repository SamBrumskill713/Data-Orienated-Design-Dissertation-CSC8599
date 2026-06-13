#include "PhysicsSystemSOA.h"
#include "CollisionDetectionSOA.h"
#include "TransformSOA.h"
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
		IntegrateAccel(realDT);

		if (data.useBroadPhase) {
			BroadPhase();
			NarrowPhase();
		}
		else {
			BasicCollisionDetection();
		}

		IntegrateVelocity(realDT);
		ClearForces();
		UpdateCollisionList();

		data.dTOffset -= realDT;
	}
}

void PhysicsSystemSOA::IntegrateAccel(float dt) {
	int count = GameObjectOpsSOA::GetObjectCount(gameWorld.gameObjects);

	for (int i = 0; i < count; ++i) {
		if (!gameWorld.gameObjects.isActive[i]) {
			continue;
		}

		float inverseMass = gameWorld.gameObjects.physics.inverseMassSOA[i];
		if (inverseMass == 0.0f) {
			continue;
		}

		Vector3 accel = gameWorld.gameObjects.physics.forceSOA[i] * inverseMass;
		if (data.applyGravity) {
			accel += data.gravity;
		}

		gameWorld.gameObjects.physics.linearVelocitySOA[i] += accel * dt;

		PhysicsOpsSOA::UpdateInertiaTensor(gameWorld.gameObjects.physics, gameWorld.gameObjects.transforms.orientations[i], i);
		Vector3 angAccel = gameWorld.gameObjects.physics.inverseInertiaTensorSOA[i] * gameWorld.gameObjects.physics.torqueSOA[i];
		gameWorld.gameObjects.physics.angularVelocitySOA[i] += angAccel * dt;
	}
}

void PhysicsSystemSOA::IntegrateVelocity(float dt) {
	int count = GameObjectOpsSOA::GetObjectCount(gameWorld.gameObjects);
	const float LINEAR_DAMPING = 0.4f * dt;
	const float ANGULAR_DAMPING = 0.4f * dt;

	for (int i = 0; i < count; ++i) {
		if (!gameWorld.gameObjects.isActive[i]) {
			continue;
		}

		if (gameWorld.gameObjects.physics.inverseMassSOA[i] == 0.0f) {
			continue;
		}

		gameWorld.gameObjects.transforms.positions[i] += gameWorld.gameObjects.physics.linearVelocitySOA[i] * dt;
		gameWorld.gameObjects.physics.linearVelocitySOA[i] *= (1.0f - LINEAR_DAMPING);

		gameWorld.gameObjects.transforms.orientations[i] = gameWorld.gameObjects.transforms.orientations[i] +
			(Quaternion(gameWorld.gameObjects.physics.angularVelocitySOA[i] * dt * 0.5f, 0.0f) * gameWorld.gameObjects.transforms.orientations[i]);
		gameWorld.gameObjects.transforms.orientations[i].Normalise();

		gameWorld.gameObjects.physics.angularVelocitySOA[i] *= (1.0f - ANGULAR_DAMPING);

		TransformOpsSOA::UpdateAllMatrices(gameWorld.gameObjects.transforms);
	}
}

void PhysicsSystemSOA::ClearForces() {
	int count = GameObjectOpsSOA::GetObjectCount(gameWorld.gameObjects);
	for (int i = 0; i < count; ++i) {
		if (gameWorld.gameObjects.isActive[i]) {
			PhysicsOpsSOA::ClearForces(gameWorld.gameObjects.physics, i);
		}
	}
}

void PhysicsSystemSOA::BroadPhase() {
	broadphasePairs.clear();

	int count = GameObjectOpsSOA::GetObjectCount(gameWorld.gameObjects);

	QuadTreeSOA<int> quadTree(Vector2(1000.0f, 1000.0f), 6, 10);

	for (int i = 0; i < count; ++i) {
		if (!gameWorld.gameObjects.isActive[i] || gameWorld.gameObjects.physics.inverseMassSOA[i] == 0.0f) {
			continue;
		}

		// For broadphase, use a simple bounding box approximation
		Vector3 halfSize(1.0f, 1.0f, 1.0f);  // Default, could be from collision volume
		quadTree.Insert(i, gameWorld.gameObjects.transforms.positions[i], halfSize * 2.0f);
	}

	std::vector<int> dynamicObjects;
	for (int i = 0; i < count; ++i) {
		if (gameWorld.gameObjects.isActive[i] && gameWorld.gameObjects.physics.inverseMassSOA[i] != 0.0f) {
			dynamicObjects.push_back(i);
		}
	}

	quadTree.OperateOnContents(
		[&](std::vector<QuadTreeEntrySOA<int>>& contents) {
			for (size_t i = 0; i < contents.size(); ++i) {
				for (size_t j = i + 1; j < contents.size(); ++j) {
					int idxA = contents[i].object;
					int idxB = contents[j].object;

					if (idxA > idxB) {
						std::swap(idxA, idxB);
					}

					broadphasePairs.push_back(BroadphasePairSOA(idxA, idxB));
				}
			}
		}
	);

	for (int i = 0; i < count; ++i) {
		if (!gameWorld.gameObjects.isActive[i] || gameWorld.gameObjects.physics.inverseMassSOA[i] != 0.0f) {
			continue;
		}

		for (int j : dynamicObjects) {
			int idxA = i;
			int idxB = j;
			if (idxA > idxB) {
				std::swap(idxA, idxB);
			}

			broadphasePairs.push_back(BroadphasePairSOA(idxA, idxB));
		}
	}

	std::sort(broadphasePairs.begin(), broadphasePairs.end());
	broadphasePairs.erase(std::unique(broadphasePairs.begin(), broadphasePairs.end()), broadphasePairs.end());
}

void PhysicsSystemSOA::NarrowPhase() {
	CollisionInfoSOA collisionInfo;

	for (const auto& pair : broadphasePairs) {
		int countA = GameObjectOpsSOA::GetObjectCount(gameWorld.gameObjects);
		int countB = countA;

		if (pair.indexA >= countA || pair.indexB >= countB) {
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

void PhysicsSystemSOA::BasicCollisionDetection() {
	int count = GameObjectOpsSOA::GetObjectCount(gameWorld.gameObjects);
	CollisionInfoSOA collisionInfo;

	for (int i = 0; i < count; ++i) {
		if (!gameWorld.gameObjects.isActive[i]) {
			continue;
		}

		for (int j = i + 1; j < count; ++j) {
			if (!gameWorld.gameObjects.isActive[j] || gameWorld.gameObjects.physics.inverseMassSOA[j] == 0.0f) {
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

void PhysicsSystemSOA::UpdateCollisionList() {
	auto& objects = gameWorld.gameObjects;
	int count = GameObjectOpsSOA::GetObjectCount(objects);

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

	PhysicsOpsSOA::ApplyAngularImpulse(physicsA, Vector::Cross(localA, -fullImpulse), indexA);
	PhysicsOpsSOA::ApplyAngularImpulse(physicsB, Vector::Cross(localB, fullImpulse), indexB);

	TransformOpsSOA::UpdateMatrixSOA(transformsA, indexA);
	TransformOpsSOA::UpdateMatrixSOA(transformsB, indexB);
}