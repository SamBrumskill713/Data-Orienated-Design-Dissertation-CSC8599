#pragma once
#include "Vector.h"
#include "Quaternion.h"
#include "Texture.h"
#include "GameWorldDOD.h"
#include "PhysicsSystemDOD.h"
#include "GameTechRendererDOD.h"
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

		struct TutorialGameData {
			std::vector<float> frameTimeSamples;
			size_t objectIndex;
			size_t floorIndex;
			float forceMagnitude;
			bool useGravity;
			int x;
			int y;
			static constexpr int FPS_SAMPLE_SIZE = 60;
			int frameCount = 0;
			float fpsUpdateTimer = 0.0f;
			float averageFPS = 0.0f;

			TutorialGameData()
				:forceMagnitude(10), objectIndex((size_t)-1), floorIndex((size_t)-1) {
			}
		};

		struct TutorialGameResources {
			Rendering::Mesh* cubeMesh;
			Rendering::Mesh* sphereMesh;

			Rendering::Texture* defaultTex;
			Rendering::Texture* checkerTex;

			GameTechMaterial checkerMaterial;

			TutorialGameResources()
				:cubeMesh(nullptr), sphereMesh(nullptr), defaultTex(nullptr), checkerTex(nullptr) {
			}
		};

		class TutorialGameDOD {
		public:
			TutorialGameDOD(GameWorldDOD& gameWorld, RendererSystemDOD& renderer, PhysicsSystemDOD& physics);
			~TutorialGameDOD();

			TutorialGameData data;
			TutorialGameResources resources;

			void InitWorld();
			void UpdateGame(float dt);
			void InitTest();

		private:
			GameWorldDOD& gameWorld;
			RendererSystemDOD& rendererDOD;
			PhysicsSystemDOD& physics;
			Controller* controller;

			TimingDisplay timingDisplay;
			PerformanceLogger performanceLogger{ "performance_AoS.csv" };

			void InitCamera();
			void LoadResources();

			size_t AddFloorToWorld(const Vector3& position, float floorHeight, float floorLength, int collisionLayer = 0);
			size_t addCubeToWorld(const Vector3& position, const Vector3& cubeDims, float inverseMass = 10.0f, int collisionLayer = 0);
			void CreateAABBGrid(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
		};
	}

}