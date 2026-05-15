#include "GameWorldDOD.h"
#include "CollisionDetectionDOD.h"

using namespace NCL;
using namespace NCL::CSC8503;

GameWorldDOD::GameWorldDOD()
	: quadTree(Vector2(1000.0f, 1000.0f), 6, 10) {
}

void GameWorldDOD::AddGameObject(GameObjectDOD& obj) {
	obj.worldID = data.worldIDCounter++;
	data.worldStateCounter++;
}

void GameWorldDOD::RemoveGameObject(size_t index) {
	gameObjects.RemoveObject(index);
	data.worldStateCounter++;
}

void GameWorldDOD::Clear() {
	gameObjects.Clear();
	data.worldIDCounter = 0;
	data.worldStateCounter = 0;
}

bool GameWorldDOD::Raycast(Ray& r, RayCollisionDOD& closestCollision, bool closestObject, size_t ignoreObjectIndex) const {
	RayCollisionDOD collision;
	bool found = false;

	const auto& objects = gameObjects.GetObjectArray();

	for (size_t i = 0; i < objects.size(); ++i) {
		if (!objects[i].isActive || i == ignoreObjectIndex) {
			continue;
		}

		RayCollisionDOD thisCollision;
		if (CollisionDetectionDOD::RayAABBIntersection(r, objects[i].transform, objects[i].collision, thisCollision)) {
			if (!closestObject) {
				closestCollision = thisCollision;
				closestCollision.node = (void*)i;
				return true;
			}
			else {
				if (thisCollision.rayDistance < collision.rayDistance) {
					collision = thisCollision;
					collision.node = (void*)i;
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

void GameWorldDOD::UpdateWorld(float dt) {
	if (data.shuffleObjects) {
		ShuffleGameObjects();
	}
}

void GameWorldDOD::ShuffleGameObjects() {
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine engine(seed);
	std::shuffle(gameObjects.GetObjectArray().begin(), gameObjects.GetObjectArray().end(), engine);
}

void GameWorldDOD::OperateOnContents(std::function<void(GameObjectDOD&)> func) {
	auto& objects = gameObjects.GetObjectArray();
	for (auto& obj : objects) {
		func(obj);
	}
}