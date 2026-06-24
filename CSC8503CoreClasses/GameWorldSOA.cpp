#include "GameWorldSOA.h"
#include "CollisionDetectionSOA.h"

using namespace NCL;
using namespace NCL::CSC8503;

GameWorldSOA::GameWorldSOA()
	: quadTree(Vector2(1000.0f, 1000.0f), 6, 10) {
}

void GameWorldSOA::AddGameObject(GameObjectType type) {
	int objIndex = GameObjectOpsSOA::AddGameObject(gameObjects, type);
	gameObjects.worldIDs[objIndex] = data.worldIDCounter++;
	data.worldStateCounter++;
}

void GameWorldSOA::RemoveGameObject(int index) {
	GameObjectOpsSOA::RemoveGameObject(gameObjects, index);
	data.worldStateCounter++;
}

void GameWorldSOA::Clear() {
	GameObjectOpsSOA::Clear(gameObjects);
	data.worldIDCounter = 0;
	data.worldStateCounter = 0;
}

void NCL::CSC8503::GameWorldSOA::reserveCapacity(int estimatedCapacity)
{
	// Reserve main object arrays
	gameObjects.worldIDs.reserve(estimatedCapacity);
	gameObjects.objectTypes.reserve(estimatedCapacity);
	gameObjects.isActive.reserve(estimatedCapacity);
	gameObjects.isCollided.reserve(estimatedCapacity);
	gameObjects.collisionLayers.reserve(estimatedCapacity);
	gameObjects.broadphaseAABBs.reserve(estimatedCapacity);

	// Reserve transform arrays
	gameObjects.transforms.positions.reserve(estimatedCapacity);
	gameObjects.transforms.matrices.reserve(estimatedCapacity);
	gameObjects.transforms.orientations.reserve(estimatedCapacity);
	gameObjects.transforms.scales.reserve(estimatedCapacity);

	// Reserve physics arrays
	gameObjects.physics.inverseInertiaTensorSOA.reserve(estimatedCapacity);
	gameObjects.physics.linearVelocitySOA.reserve(estimatedCapacity);
	gameObjects.physics.forceSOA.reserve(estimatedCapacity);
	gameObjects.physics.angularVelocitySOA.reserve(estimatedCapacity);
	gameObjects.physics.torqueSOA.reserve(estimatedCapacity);
	gameObjects.physics.inverseInertiaSOA.reserve(estimatedCapacity);
	gameObjects.physics.inverseMassSOA.reserve(estimatedCapacity);
	gameObjects.physics.elasticitySOA.reserve(estimatedCapacity);
	gameObjects.physics.frictionSOA.reserve(estimatedCapacity);
	gameObjects.physics.isCollidedSOA.reserve(estimatedCapacity);

	// Reserve render arrays
	gameObjects.render.meshes.reserve(estimatedCapacity);
	gameObjects.render.materialTypes.reserve(estimatedCapacity);
	gameObjects.render.diffuseTextures.reserve(estimatedCapacity);
	gameObjects.render.bumpTextures.reserve(estimatedCapacity);
	gameObjects.render.colours.reserve(estimatedCapacity);

	// Reserve collision arrays
	gameObjects.collision.dataSOA.typeSOA.reserve(estimatedCapacity);
	gameObjects.collision.dataSOA.collisionLayerSOA.reserve(estimatedCapacity);
	gameObjects.collision.dataSOA.isTriggerSOA.reserve(estimatedCapacity);
	gameObjects.collision.AABBDataSOA.halfSizesSOA.reserve(estimatedCapacity);
	gameObjects.collision.typeDataIndex.reserve(estimatedCapacity);
}

bool GameWorldSOA::Raycast(Ray& r, RayCollisionSOA& closestCollision, bool closestObject, int ignoreObjectIndex) const {
	RayCollisionSOA collision;
	bool found = false;

	int objectCount = GameObjectOpsSOA::GetObjectCount(gameObjects);

	for (int i = 0; i < objectCount; ++i) {
		if (!gameObjects.isActive[i] || i == ignoreObjectIndex) {
			continue;
		}

		RayCollisionSOA thisCollision;
		if (CollisionDetectionSOA::RayAABBIntersection(r, i, gameObjects, thisCollision)) {
			if (!closestObject) {
				closestCollision = thisCollision;
				closestCollision.node = (void*)(intptr_t)i;
				return true;
			}
			else {
				if (thisCollision.rayDistance < collision.rayDistance) {
					collision = thisCollision;
					collision.node = (void*)(intptr_t)i;
					found = true;
				}
			}
		}
	}

	if (found) {
		closestCollision = collision;
		return true;
	}

	return false;
}

void GameWorldSOA::UpdateWorld(float dt) {
	if (data.shuffleObjects) {
		ShuffleGameObjects();
	}
}

void GameWorldSOA::ShuffleGameObjects() {
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine engine(seed);

	int count = GameObjectOpsSOA::GetObjectCount(gameObjects);
	std::vector<int> indices(count);
	for (int i = 0; i < count; ++i) {
		indices[i] = i;
	}
	std::shuffle(indices.begin(), indices.end(), engine);

	std::vector<int> tempWorldIDs = gameObjects.worldIDs;
	std::vector<GameObjectType> tempTypes = gameObjects.objectTypes;
	std::vector<bool> tempActive = gameObjects.isActive;
	std::vector<bool> tempCollided = gameObjects.isCollided;
	std::vector<int> tempLayers = gameObjects.collisionLayers;
	std::vector<Vector3> tempBroadphaseAABBs = gameObjects.broadphaseAABBs;

	std::vector<Matrix4> tempMatrices = gameObjects.transforms.matrices;
	std::vector<Quaternion> tempOrientations = gameObjects.transforms.orientations;
	std::vector<Vector3> tempPositions = gameObjects.transforms.positions;
	std::vector<Vector3> tempScales = gameObjects.transforms.scales;

	std::vector<Matrix3> tempInertiaTensors = gameObjects.physics.inverseInertiaTensorSOA;
	std::vector<Vector3> tempLinearVel = gameObjects.physics.linearVelocitySOA;
	std::vector<Vector3> tempForce = gameObjects.physics.forceSOA;
	std::vector<Vector3> tempAngularVel = gameObjects.physics.angularVelocitySOA;
	std::vector<Vector3> tempTorque = gameObjects.physics.torqueSOA;
	std::vector<Vector3> tempInverseInertia = gameObjects.physics.inverseInertiaSOA;
	std::vector<float> tempInverseMass = gameObjects.physics.inverseMassSOA;
	std::vector<float> tempElasticity = gameObjects.physics.elasticitySOA;
	std::vector<float> tempFriction = gameObjects.physics.frictionSOA;
	std::vector<bool> tempPhysicsCollided = gameObjects.physics.isCollidedSOA;

	std::vector<Mesh*> tempMeshes = gameObjects.render.meshes;
	std::vector<MaterialType> tempMatTypes = gameObjects.render.materialTypes;
	std::vector<Texture*> tempDiffuseTex = gameObjects.render.diffuseTextures;
	std::vector<Texture*> tempBumpTex = gameObjects.render.bumpTextures;
	std::vector<Vector4> tempColours = gameObjects.render.colours;

	for (int i = 0; i < count; ++i) {
		int oldIdx = indices[i];

		gameObjects.worldIDs[i] = tempWorldIDs[oldIdx];
		gameObjects.objectTypes[i] = tempTypes[oldIdx];
		gameObjects.isActive[i] = tempActive[oldIdx];
		gameObjects.isCollided[i] = tempCollided[oldIdx];
		gameObjects.collisionLayers[i] = tempLayers[oldIdx];
		gameObjects.broadphaseAABBs[i] = tempBroadphaseAABBs[oldIdx];

		gameObjects.transforms.matrices[i] = tempMatrices[oldIdx];
		gameObjects.transforms.orientations[i] = tempOrientations[oldIdx];
		gameObjects.transforms.positions[i] = tempPositions[oldIdx];
		gameObjects.transforms.scales[i] = tempScales[oldIdx];

		gameObjects.physics.inverseInertiaTensorSOA[i] = tempInertiaTensors[oldIdx];
		gameObjects.physics.linearVelocitySOA[i] = tempLinearVel[oldIdx];
		gameObjects.physics.forceSOA[i] = tempForce[oldIdx];
		gameObjects.physics.angularVelocitySOA[i] = tempAngularVel[oldIdx];
		gameObjects.physics.torqueSOA[i] = tempTorque[oldIdx];
		gameObjects.physics.inverseInertiaSOA[i] = tempInverseInertia[oldIdx];
		gameObjects.physics.inverseMassSOA[i] = tempInverseMass[oldIdx];
		gameObjects.physics.elasticitySOA[i] = tempElasticity[oldIdx];
		gameObjects.physics.frictionSOA[i] = tempFriction[oldIdx];
		gameObjects.physics.isCollidedSOA[i] = tempPhysicsCollided[oldIdx];

		gameObjects.render.meshes[i] = tempMeshes[oldIdx];
		gameObjects.render.materialTypes[i] = tempMatTypes[oldIdx];
		gameObjects.render.diffuseTextures[i] = tempDiffuseTex[oldIdx];
		gameObjects.render.bumpTextures[i] = tempBumpTex[oldIdx];
		gameObjects.render.colours[i] = tempColours[oldIdx];
	}
}

void GameWorldSOA::OperateOnContents(std::function<void(int)> func) {
	int count = GameObjectOpsSOA::GetObjectCount(gameObjects);
	for (int i = 0; i < count; ++i) {
		func(i);
	}
}