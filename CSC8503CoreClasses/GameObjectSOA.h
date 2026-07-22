#pragma once

#include <vector>
#include <string>
#include "TransformSOA.h"
#include "CollisionVolumeSOA.h"
#include "PhysicsObjectSOA.h"
#include "RenderObjectSOA.h"
#include "GameObjectDOD.h"

using namespace NCL::Maths;

namespace NCL::CSC8503 {
	struct GameObjectCompSOA {
		std::vector<int> worldIDs;
		std::vector<GameObjectType> objectTypes;
		std::vector<char> isActive;
		std::vector<char> isCollided;
		std::vector<int> collisionLayers;
		std::vector<Vector3> broadphaseAABBs;

		TransformCompSOA transforms;
		PhysicsObjectCompSOA physics;
		RenderObjectCompSOA render;
		CollisionVolumeSysSOA collision;

		// GARBAGE DATA - Separate arrays that don't pollute hot data cache lines
		// Each array holds 256 bytes per entity
		std::vector<uint8_t> garbageData1;  // 256 bytes per entity
		std::vector<uint8_t> garbageData2;  // 256 bytes per entity
	};

	namespace GameObjectOpsSOA {

		inline int GetObjectCount(const GameObjectCompSOA& gameObjects) {
			return gameObjects.worldIDs.size();
		}

		inline int AddGameObject(GameObjectCompSOA& gameObjects,
			GameObjectType objectType = GameObjectType::Default) {
			int index = GetObjectCount(gameObjects);

			gameObjects.worldIDs.emplace_back(-1);
			gameObjects.objectTypes.emplace_back(objectType);
			gameObjects.isActive.emplace_back(true);
			gameObjects.isCollided.emplace_back(false);
			gameObjects.collisionLayers.emplace_back(0);
			gameObjects.broadphaseAABBs.emplace_back(Vector3(0, 0, 0));

			TransformOpsSOA::AddTransform(gameObjects.transforms);
			PhysicsOpsSOA::AddPhysicsBody(gameObjects.physics);
			RenderOpsSOA::AddRenderObject(gameObjects.render);

			gameObjects.collision.CreateAABB(Vector3(0.5f, 0.5f, 0.5f), 0, false);

			// Add garbage data (512 bytes per entity: 256 + 256)
			for (int i = 0; i < 256; ++i) {
				gameObjects.garbageData1.emplace_back(0xAB);
				gameObjects.garbageData2.emplace_back(0xCD);
			}

			return index;
		}

		inline void RemoveGameObject(GameObjectCompSOA& gameObjects, int index) {
			if (index < 0 || index >= GetObjectCount(gameObjects)) return;

			int lastIndex = GetObjectCount(gameObjects) - 1;

			if (index != lastIndex) {
				gameObjects.worldIDs[index] = gameObjects.worldIDs[lastIndex];
				gameObjects.objectTypes[index] = gameObjects.objectTypes[lastIndex];
				gameObjects.isActive[index] = gameObjects.isActive[lastIndex];
				gameObjects.isCollided[index] = gameObjects.isCollided[lastIndex];
				gameObjects.collisionLayers[index] = gameObjects.collisionLayers[lastIndex];
				gameObjects.broadphaseAABBs[index] = gameObjects.broadphaseAABBs[lastIndex];
			}

			gameObjects.worldIDs.pop_back();
			gameObjects.objectTypes.pop_back();
			gameObjects.isActive.pop_back();
			gameObjects.isCollided.pop_back();
			gameObjects.collisionLayers.pop_back();
			gameObjects.broadphaseAABBs.pop_back();

			TransformOpsSOA::RemoveTransform(gameObjects.transforms, index);
			PhysicsOpsSOA::RemovePhysicsBody(gameObjects.physics, index);
			RenderOpsSOA::RemoveRenderObject(gameObjects.render, index);
			gameObjects.collision.RemoveCollisionVolume(index);

			// Remove garbage data
			size_t startPos = index * 256;
			size_t endPos = startPos + 256;
			if (endPos <= gameObjects.garbageData1.size()) {
				gameObjects.garbageData1.erase(gameObjects.garbageData1.begin() + startPos,
					gameObjects.garbageData1.begin() + endPos);
				gameObjects.garbageData2.erase(gameObjects.garbageData2.begin() + startPos,
					gameObjects.garbageData2.begin() + endPos);
			}
		}

		inline void GetObjectsByType(const GameObjectCompSOA& gameObjects, GameObjectType type, std::vector<int>& outIndices) {
			outIndices.clear();
			int count = GetObjectCount(gameObjects);
			for (int i = 0; i < count; ++i) {
				if (gameObjects.objectTypes[i] == type) {
					outIndices.emplace_back(i);
				}
			}
		}

		inline void GetActiveObjects(const GameObjectCompSOA& gameObjects, std::vector<int>& outIndices) {
			outIndices.clear();
			int count = GetObjectCount(gameObjects);
			for (int i = 0; i < count; ++i) {
				if (gameObjects.isActive[i]) {
					outIndices.emplace_back(i);
				}
			}
		}

		inline void Clear(GameObjectCompSOA& gameObjects) {
			gameObjects.worldIDs.clear();
			gameObjects.objectTypes.clear();
			gameObjects.isActive.clear();
			gameObjects.isCollided.clear();
			gameObjects.collisionLayers.clear();
			gameObjects.broadphaseAABBs.clear();
			gameObjects.transforms.positions.clear();
			gameObjects.transforms.orientations.clear();
			gameObjects.transforms.scales.clear();
			gameObjects.transforms.matrices.clear();
			gameObjects.physics.inverseInertiaTensorSOA.clear();
			gameObjects.physics.linearVelocitySOA.clear();
			gameObjects.physics.forceSOA.clear();
			gameObjects.physics.angularVelocitySOA.clear();
			gameObjects.physics.torqueSOA.clear();
			gameObjects.physics.inverseInertiaSOA.clear();
			gameObjects.physics.inverseMassSOA.clear();
			gameObjects.physics.elasticitySOA.clear();
			gameObjects.physics.frictionSOA.clear();
			gameObjects.physics.isCollidedSOA.clear();
			gameObjects.render.meshes.clear();
			gameObjects.render.materialTypes.clear();
			gameObjects.render.diffuseTextures.clear();
			gameObjects.render.bumpTextures.clear();
			gameObjects.render.colours.clear();
			gameObjects.collision.dataSOA.typeSOA.clear();
			gameObjects.collision.dataSOA.collisionLayerSOA.clear();
			gameObjects.collision.dataSOA.isTriggerSOA.clear();
			gameObjects.collision.AABBDataSOA.halfSizesSOA.clear();
			gameObjects.collision.typeDataIndex.clear();
			gameObjects.garbageData1.clear();
			gameObjects.garbageData2.clear();
		}
	}
}