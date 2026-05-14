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

	struct PickUpComp {
		int type;
		int pointValue;
		bool isRendered;

		PickUpComp() : type(0), pointValue(1), isRendered(true) {}
	};

	struct PlayerComp {
		Vector3 respawnPosition;
		int score;
		bool hasPickup;
		std::vector<size_t> carriedPickupIndices;

		PlayerComp() : respawnPosition(Vector3()), score(0), hasPickup(false) {}
	};

	struct ObstacleComp {
		bool destructible;
		ObstacleComp() : destructible(false) {}
	};

	struct TriggerComp {
		bool isActive;
		TriggerComp() : isActive(true) {}
	};

	// Main game object - Array of Structs approach (AABB collision only)
	struct GameObjectDOD {
		// CORE COMPONENTS
		TransformsComp transform;
		AABBComp collision;              // AABB collision data only
		PhysicsObjectComp physics;
		RenderObjectComp render;

		// TYPE-SPECIFIC COMPONENTS
		PickUpComp pickUp;
		PlayerComp player;
		ObstacleComp obstacle;
		TriggerComp trigger;

		// METADATA
		std::string name;
		bool isActive;
		bool isCollided;
		int worldID;
		Vector3 broadphaseAABB;
		GameObjectType objectType;
		int collisionLayer;

		// OPTIONAL COMPONENTS
		void* networkObject;

		GameObjectDOD()
			: name(""), isActive(true), isCollided(false), worldID(-1),
			broadphaseAABB(Vector3(0, 0, 0)), objectType(GameObjectType::Default),
			collisionLayer(0), networkObject(nullptr) {
		}

		GameObjectDOD(const std::string& objName, GameObjectType type = GameObjectType::Default)
			: name(objName), isActive(true), isCollided(false), worldID(-1),
			broadphaseAABB(Vector3(0, 0, 0)), objectType(type),
			collisionLayer(0), networkObject(nullptr) {
		}
	};

	// Simple array storage for AoS
	struct GameObjectStorage {
		std::vector<GameObjectDOD> objects;

		GameObjectDOD& AddObject(const std::string& name = "", GameObjectType type = GameObjectType::Default) {
			objects.emplace_back(name, type);
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
					outIndices.push_back(i);
				}
			}
		}

		void GetActiveObjects(std::vector<size_t>& outIndices) const {
			outIndices.clear();
			for (size_t i = 0; i < objects.size(); ++i) {
				if (objects[i].isActive) {
					outIndices.push_back(i);
				}
			}
		}

		void Clear() {
			objects.clear();
		}
	};
}