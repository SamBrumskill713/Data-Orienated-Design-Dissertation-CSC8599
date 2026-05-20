#include "PhysicsSystemDOD.h"
#include "CollisionDetectionDOD.h"
#include "TransformDOD.h"
#include <algorithm>

using namespace NCL;
using namespace NCL::CSC8503;

const int IDEAL_HZ = 60;
const float IDEAL_DT = 1.0f / IDEAL_HZ;

PhysicsSystemDOD::PhysicsSystemDOD(GameWorldDOD& world)
	: gameWorld(world) {
}

void PhysicsSystemDOD::Clear() {
	activeCollisions.clear();
	broadphasePairs.clear();
	data.dTOffset = 0.0f;
}

void PhysicsSystemDOD::Update(float dt) {
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

void PhysicsSystemDOD::IntegrateAccel(float dt) {
	auto& objects = gameWorld.gameObjects.GetObjectArray();

	for (auto& obj : objects) {
		if (!obj.isActive) {
			continue;
		}

		float inverseMass = obj.physics.inverseMass;
		if (inverseMass == 0.0f) {
			continue;
		}

		Vector3 accel = obj.physics.force * inverseMass;
		if (data.applyGravity) {
			accel += data.gravity;
		}

		obj.physics.linearVelocity += accel * dt;

		PhysicsOps::UpdateInertiaTensor(obj.physics, obj.transform.orientation);
		Vector3 angAccel = obj.physics.inverseInertiaTensor * obj.physics.torque;
		obj.physics.angularVelocity += angAccel * dt;
	}
}

void PhysicsSystemDOD::IntegrateVelocity(float dt) {
	auto& objects = gameWorld.gameObjects.GetObjectArray();
	const float LINEAR_DAMPING = 0.4f * dt;
	const float ANGULAR_DAMPING = 0.4f * dt;

	for (auto& obj : objects) {
		if (!obj.isActive) {
			continue;
		}

		if (obj.physics.inverseMass == 0.0f) {
			continue; 
		}

		obj.transform.position += obj.physics.linearVelocity * dt;
		obj.physics.linearVelocity *= (1.0f - LINEAR_DAMPING);

		obj.transform.orientation = obj.transform.orientation +
			(Quaternion(obj.physics.angularVelocity * dt * 0.5f, 0.0f) * obj.transform.orientation);
		obj.transform.orientation.Normalise();

		obj.physics.angularVelocity *= (1.0f - ANGULAR_DAMPING);

		TransformOps::UpdateMatrix(obj.transform);
	}
}

void PhysicsSystemDOD::ClearForces() {
	auto& objects = gameWorld.gameObjects.GetObjectArray();
	for (auto& obj : objects) {
		if (obj.isActive) {
			PhysicsOps::ClearForces(obj.physics);
		}
	}
}

void PhysicsSystemDOD::BroadPhase() {
	broadphasePairs.clear();
	
	auto& objects = gameWorld.gameObjects.GetObjectArray();

	QuadTreeDOD<size_t> quadTree(Vector2(1000.0f, 1000.0f), 6, 10);

	for (size_t i = 0; i < objects.size(); ++i) {
		if (!objects[i].isActive) {
			continue;
		}

		Vector3 halfSize = objects[i].collision.halfSizes;
		quadTree.Insert(i, objects[i].transform.position, halfSize * 2.0f);
	}

	quadTree.OperateOnContents(
		[&](std::vector<QuadTreeEntryDOD<size_t>>& contents) {
			for (size_t i = 0; i < contents.size(); ++i) {
				for (size_t j = i + 1; j < contents.size(); ++j) {
					size_t idxA = contents[i].object;
					size_t idxB = contents[j].object;

					if (idxA > idxB) {
						std::swap(idxA, idxB);
					}

					broadphasePairs.push_back(BroadphasePair(idxA, idxB));
				}
			}
		}
	);

	std::sort(broadphasePairs.begin(), broadphasePairs.end());
	broadphasePairs.erase(std::unique(broadphasePairs.begin(), broadphasePairs.end()), broadphasePairs.end());
}

void PhysicsSystemDOD::NarrowPhase() {
	auto& objects = gameWorld.gameObjects.GetObjectArray();
	CollisionInfoDOD collisionInfo;

	for (const auto& pair : broadphasePairs) {
		if (pair.indexA >= objects.size() || pair.indexB >= objects.size()) {
			continue;
		}

		GameObjectDOD& objA = objects[pair.indexA];
		GameObjectDOD& objB = objects[pair.indexB];

		if (!objA.isActive || !objB.isActive) {
			continue;
		}

		if (CollisionDetectionDOD::ObjectIntersection(objA, objB, pair.indexA, pair.indexB, collisionInfo)) {
			objA.isCollided = true;
			objB.isCollided = true;

			ImpulseResolveCollision(objA.physics, objB.physics, objA.transform, objB.transform,
				collisionInfo.normal, collisionInfo.localA, collisionInfo.localB, collisionInfo.penetration);

			activeCollisions.push_back(
				ActiveCollisionDOD(pair.indexA, pair.indexB, data.numCollisionFrames)
			);
		}
	}
}

void PhysicsSystemDOD::BasicCollisionDetection() {
	auto& objects = gameWorld.gameObjects.GetObjectArray();
	CollisionInfoDOD collisionInfo;

	for (size_t i = 0; i < objects.size(); ++i) {
		if (!objects[i].isActive) {
			continue;
		}

		for (size_t j = i + 1; j < objects.size(); ++j) {
			if (!objects[j].isActive || objects[j].physics.inverseMass == 0.0f) {
				continue;
			}

			if (CollisionDetectionDOD::ObjectIntersection(objects[i], objects[j], i, j, collisionInfo)) {
				objects[i].isCollided = true;
				objects[j].isCollided = true;

				ImpulseResolveCollision(objects[i].physics, objects[j].physics, objects[i].transform, objects[j].transform,
					collisionInfo.normal, collisionInfo.localA, collisionInfo.localB, collisionInfo.penetration);

				activeCollisions.push_back(
					ActiveCollisionDOD(i, j, data.numCollisionFrames)
				);
			}
		}
	}
}

void PhysicsSystemDOD::UpdateCollisionList() {
	auto& objects = gameWorld.gameObjects.GetObjectArray();

	for (auto it = activeCollisions.begin(); it != activeCollisions.end(); ) {
		it->framesLeft--;

		if (it->framesLeft < 0) {
			it = activeCollisions.erase(it);
		}
		else {
			++it;
		}
	}

	for (auto& obj : objects) {
		obj.isCollided = false;
	}
}

void PhysicsSystemDOD::ImpulseResolveCollision(
	PhysicsObjectComp& physA, PhysicsObjectComp& physB,
	TransformsComp& transA, TransformsComp& transB,
	const Vector3& normal, const Vector3& localA, const Vector3& localB, float penetration) const {

	float totalMass = physA.inverseMass + physB.inverseMass;
	if (totalMass == 0.0f) {
		return; 
	}

	transA.position -= normal * penetration * (physA.inverseMass / totalMass);
	transB.position += normal * penetration * (physB.inverseMass / totalMass);

	Vector3 angVelA = Vector::Cross(physA.angularVelocity, localA);
	Vector3 angVelB = Vector::Cross(physB.angularVelocity, localB);

	Vector3 fullVelA = physA.linearVelocity + angVelA;
	Vector3 fullVelB = physB.linearVelocity + angVelB;

	Vector3 contactVel = fullVelB - fullVelA;
	float impulseForce = Vector::Dot(contactVel, normal);

	Vector3 inertiaA = Vector::Cross(physA.inverseInertiaTensor *
		Vector::Cross(localA, normal), localA);
	Vector3 inertiaB = Vector::Cross(physB.inverseInertiaTensor *
		Vector::Cross(localB, normal), localB);

	float angularEffect = Vector::Dot(inertiaA + inertiaB, normal);

	const float restitution = 0.66f;
	float j = (-(1.0f + restitution) * impulseForce) / (totalMass + angularEffect);

	Vector3 fullImpulse = normal * j;

	PhysicsOps::ApplyLinearImpulse(physA, -fullImpulse);
	PhysicsOps::ApplyLinearImpulse(physB, fullImpulse);

	PhysicsOps::ApplyAngularImpulse(physA, Vector::Cross(localA, -fullImpulse));
	PhysicsOps::ApplyAngularImpulse(physB, Vector::Cross(localB, fullImpulse));

	TransformOps::UpdateMatrix(transA);
	TransformOps::UpdateMatrix(transB);
}