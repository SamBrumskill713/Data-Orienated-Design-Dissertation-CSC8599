#pragma once
#include "RenderObject.h"
#include "../CSC8503CoreClasses/CollisionVolume.h"
#include "../CSC8503CoreClasses/GameObject.h"
#include "TimingDisplay.h"
#include "PerformanceLogger.h"
#include <chrono>

namespace NCL {
	class Controller;

	namespace Rendering {
		class Mesh;
		class Texture;
		class Shader;
	}
	namespace CSC8503 {
		class GameTechRendererInterface;
		class PhysicsSystem;
		class GameWorld;
		class GameObject;

		class TutorialGame {
		public:
			TutorialGame(GameWorld& gameWorld, GameTechRendererInterface& renderer, PhysicsSystem& physics);
			~TutorialGame();
			void InitWorld();
			virtual void UpdateGame(float dt);

		protected:
			void InitCamera();

			void InitFPSTest();

			/*
			These are some of the world/object creation functions I created when testing the functionality
			in the module. Feel free to mess around with them to see different objects being created in different
			test scenarios (constraints, collision types, and so on).
			*/
			void CreateAABBGrid(int numRows, int numCols, float rowSpacing, float colSpacing, const NCL::Maths::Vector3& cubeDims);

			GameObject* AddFloorToWorld(const NCL::Maths::Vector3& position, float floorHeight, float floorLength,
				bool isTrigger = false, int collisionLayer = terrainLayer);

			GameObject* AddCubeToWorld(const NCL::Maths::Vector3& position, NCL::Maths::Vector3 dimensions,
				float inverseMass = 10.0f, bool isTrigger = false, int collisionLayer = defaultLayer);

			GameWorld& world;
			GameTechRendererInterface& renderer;
			PhysicsSystem& physics;
			Controller* controller;

			bool useGravity;
			float forceMagnitude;

			Rendering::Mesh* cubeMesh = nullptr;
			Rendering::Texture* checkerTex = nullptr;

			GameTechMaterial checkerMaterial;

			TimingDisplay timingDisplay;

			PerformanceLogger performanceLogger{ "performance_OOP.csv" };
		};
	}
}