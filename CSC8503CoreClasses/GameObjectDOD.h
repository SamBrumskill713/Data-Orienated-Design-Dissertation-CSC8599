#pragma once
#include <vector>
#include <string>
#include "TransformDOD.h"
#include "CollisionVolumeDOD.h"
#include "PhysicsObjectDOD.h"
#include "RenderObjectDOD.h"

using namespace NCL::Maths;

namespace NCL::CSC8503 {

	enum class GameObjectType : uint8_t {
		Default = 0,
		Trigger = 1,
		PickUp = 2,
		Player = 3,
		Obstacle = 4
	};

	struct GameObjectDOD {
		PhysicsObjectComp physics;
		TransformsComp transform;        
		RenderObjectComp render;
		Vector3 broadphaseAABB;
		AABBComp collision;
		int worldID;
		int collisionLayer;
		bool isActive;
		bool isCollided;
		GameObjectType objectType;

		GameObjectDOD()
			:isActive(true), isCollided(false), worldID(-1),
			broadphaseAABB(Vector3(0, 0, 0)), objectType(GameObjectType::Default),
			collisionLayer(0){
		}

		GameObjectDOD(const std::string& objName, GameObjectType type = GameObjectType::Default)
			:isActive(true), isCollided(false), worldID(-1),
			broadphaseAABB(Vector3(0, 0, 0)), objectType(type),
			collisionLayer(0){
		}
	};

	struct GameObjectStorage {
		std::vector<GameObjectDOD> objects;

		GameObjectDOD& AddObject(GameObjectType type = GameObjectType::Default) {
			objects.emplace_back();
			objects.back().objectType = type;
			return objects.back();
		}

		void RemoveObject(size_t index) {
			if (index < objects.size()) {
				objects.erase(objects.begin() + index);
			}
		}

		size_t GetObjectCount() const {
			return objects.size();
		}

		GameObjectDOD& GetObject(size_t index) {
			return objects[index];
		}

		const GameObjectDOD& GetObject(size_t index) const {
			return objects[index];
		}

		std::vector<GameObjectDOD>& GetObjectArray() {
			return objects;
		}

		const std::vector<GameObjectDOD>& GetObjectArray() const {
			return objects;
		}

		void GetObjectsByType(GameObjectType type, std::vector<size_t>& outIndices) const {
			outIndices.clear();
			for (size_t i = 0; i < objects.size(); ++i) {
				if (objects[i].objectType == type) {
					outIndices.emplace_back(i);
				}
			}
		}

		void GetActiveObjects(std::vector<size_t>& outIndices) const {
			outIndices.clear();
			for (size_t i = 0; i < objects.size(); ++i) {
				if (objects[i].isActive) {
					outIndices.emplace_back(i);
				}
			}
		}

		void Clear() {
			objects.clear();
		}
	};
}