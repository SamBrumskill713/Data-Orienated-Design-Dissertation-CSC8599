#pragma once

#include <vector>
#include <string>
#include "TransformSOA.h"
#include "CollisionVolumeSOA.h"
#include "PhysicsObjectSOA.h"
#include "RenderObjectSOA.h"

using namespace NCL::Maths;

namespace NCL::CSC8503 {

	enum class GameObjectType : uint8_t {
		Default = 0,
		Trigger = 1,
		PickUp = 2,
		Player = 3,
		Obstacle = 4
	};

	struct GameObjectCompSOA {
		std::vector<int> worldIDs;
		std::vector<GameObjectType> objectTypes;
		std::vector<bool> isActive;
		std::vector<bool> isCollided;
		std::vector<int> collisionLayers;
		std::vector<Vector3> broadphaseAABBs;

		TransformCompSOA transforms;
		PhysicsObjectCompSOA physics;
		RenderObjectCompSOA render;
		CollisionVolumeSysSOA collision;
	};

	namespace GameObjectOpsSOA {

		inline int GetObjectCount(const GameObjectCompSOA& gameObjects) {
			return gameObjects.worldIDs.size();
		}

		inline int AddGameObject(GameObjectCompSOA& gameObjects,
			GameObjectType objectType = GameObjectType::Default) {
			int index = GetObjectCount(gameObjects);

			gameObjects.worldIDs.push_back(-1);
			gameObjects.objectTypes.push_back(objectType);
			gameObjects.isActive.push_back(true);
			gameObjects.isCollided.push_back(false);
			gameObjects.collisionLayers.push_back(0);
			gameObjects.broadphaseAABBs.push_back(Vector3(0, 0, 0));

			TransformOpsSOA::AddTransform(gameObjects.transforms);
			PhysicsOpsSOA::AddPhysicsBody(gameObjects.physics);
			RenderOpsSOA::AddRenderObject(gameObjects.render);

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
		}

		inline void GetObjectsByType(const GameObjectCompSOA& gameObjects, GameObjectType type, std::vector<int>& outIndices) {
			outIndices.clear();
			int count = GetObjectCount(gameObjects);
			for (int i = 0; i < count; ++i) {
				if (gameObjects.objectTypes[i] == type) {
					outIndices.push_back(i);
				}
			}
		}

		inline void GetActiveObjects(const GameObjectCompSOA& gameObjects, std::vector<int>& outIndices) {
			outIndices.clear();
			int count = GetObjectCount(gameObjects);
			for (int i = 0; i < count; ++i) {
				if (gameObjects.isActive[i]) {
					outIndices.push_back(i);
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
		}
	}
}