#pragma once

#include <vector>
#include <functional>
#include <random>
#include <chrono>
#include <algorithm>
#include "Camera.h"
#include "Ray.h"
#include "Vector.h"
#include "GameObjectSOA.h"
#include "CollisionDetectionSOA.h"
#include "QuadTreeSOA.h"

using namespace NCL::Maths;

namespace NCL::CSC8503 {
	struct GameWorldDataSOA {
		Vector3 sunPosition;
		Vector3 sunColour;
		int worldIDCounter;
		int worldStateCounter;
		bool shuffleObjects;
		bool shuffleConstraints;

		GameWorldDataSOA()
			: sunPosition(Vector3(0, 1, 0)), sunColour(Vector3(1, 1, 1)),
			worldIDCounter(0), worldStateCounter(0),
			shuffleObjects(false), shuffleConstraints(false) {
		}
	};

	class GameWorldSOA {
	public:
		GameWorldSOA();
		~GameWorldSOA() = default;

		GameWorldDataSOA data;
		GameObjectCompSOA gameObjects;
		PerspectiveCamera mainCamera;
		QuadTreeSOA<int> quadTree;

		void AddGameObject(GameObjectType type);
		void RemoveGameObject(int index);
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
			return GameObjectOpsSOA::GetObjectCount(gameObjects);
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

		void reserveCapacity(int estimatedCapacity);

		bool Raycast(Ray& r, RayCollisionSOA& closestCollision, bool closestObject = false, int ignoreObjectIndex = -1) const;

		void UpdateWorld(float dt);

		void OperateOnContents(std::function<void(int)> func);

	private:
		void ShuffleGameObjects();
	};
}