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

	// Garbage data structure to simulate padding/overhead
	// This represents unused/unused data that still takes up cache lines
	struct GarbageDataDOD {
		static constexpr size_t GARBAGE_SIZE = 256;  // 256 bytes of padding per object
		uint8_t padding[GARBAGE_SIZE] = {};

		GarbageDataDOD() {
			// Fill with recognizable pattern for analysis
			for (size_t i = 0; i < GARBAGE_SIZE; ++i) {
				padding[i] = static_cast<uint8_t>(0xAB);
			}
		}
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

		// GARBAGE DATA - Simulates real-world overhead
		// This padding is loaded into cache with every object access
		GarbageDataDOD garbageData;

		GameObjectDOD()
			:isActive(true), isCollided(false), worldID(-1),
			broadphaseAABB(Vector3(0, 0, 0)), objectType(GameObjectType::Default),
			collisionLayer(0) {
		}

		GameObjectDOD(const std::string& objName, GameObjectType type = GameObjectType::Default)
			:isActive(true), isCollided(false), worldID(-1),
			broadphaseAABB(Vector3(0, 0, 0)), objectType(type),
			collisionLayer(0) {
		}
	};

	struct GameObjectStorage {
		std::vector<GameObjectDOD> objects;

		GameObjectDOD& AddObject(GameObjectType type = GameObjectType::Default) {
			objects.emplace_back();
			objects.back().objectType = type;
			return objects.back();
		}

		// Swap-and-pop removal: O(1), maintains cache locality
		size_t RemoveObject(size_t index) {
			if (index >= objects.size()) {
				return static_cast<size_t>(-1);
			}

			size_t lastIndex = objects.size() - 1;
			if (index != lastIndex) {
				std::swap(objects[index], objects[lastIndex]);
				objects.pop_back();
				return index;
			}

			objects.pop_back();
			return static_cast<size_t>(-1);
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
				if (objects[i].isActive && objects[i].objectType == type) {
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