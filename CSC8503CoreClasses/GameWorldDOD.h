#pragma once

#include <vector>
#include <functional>
#include <random>
#include <chrono>
#include <algorithm>
#include "Camera.h"
#include "Ray.h"
#include "Vector.h"
#include "GameObjectDOD.h"
#include "CollisionDetectionDOD.h"
#include "QuadTreeDOD.h"

using namespace NCL::Maths;

namespace NCL::CSC8503 {
	struct GameWorldData {
		Vector3 sunPosition;
		Vector3 sunColour;
		int worldIDCounter;
		int worldStateCounter;
		bool shuffleObjects;
		bool shuffleConstraints;

		GameWorldData()
			: sunPosition(Vector3(0, 1, 0)), sunColour(Vector3(1, 1, 1)),
			worldIDCounter(0), worldStateCounter(0),
			shuffleObjects(false), shuffleConstraints(false) {
		}
	};

	class GameWorldDOD {
	public:
		GameWorldDOD();
		~GameWorldDOD() = default;

		GameWorldData data;
		GameObjectStorage gameObjects;
		PerspectiveCamera mainCamera;
		QuadTreeDOD<size_t> quadTree;

		void AddGameObject(GameObjectDOD& obj);
		void RemoveGameObject(size_t index);
		void Clear();

		PerspectiveCamera& GetMainCamera() {
			return mainCamera;
		}

		void ShuffleObjects(bool state) {
			data.shuffleObjects = state;
		}

		void ShuffleConstraints(bool state) {
			data.shuffleConstraints = state;
		}

		int GetWorldStateID() const {
			return data.worldStateCounter;
		}

		int GetObjectCount() const {
			return (int)gameObjects.GetObjectCount();
		}

		void SetSunPosition(const Vector3& pos) {
			data.sunPosition = pos;
		}

		Vector3 GetSunPosition() const {
			return data.sunPosition;
		}

		void SetSunColour(const Vector3& col) {
			data.sunColour = col;
		}

		Vector3 GetSunColour() const {
			return data.sunColour;
		}

		bool Raycast(Ray& r, RayCollisionDOD& closestCollision, bool closestObject = false, size_t ignoreObjectIndex = (size_t)-1) const;

		void UpdateWorld(float dt);

		void OperateOnContents(std::function<void(GameObjectDOD&)> func);


	private:
		void ShuffleGameObjects();
	};
}