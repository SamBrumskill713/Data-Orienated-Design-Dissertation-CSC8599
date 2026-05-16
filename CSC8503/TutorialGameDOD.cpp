#pragma once
#include "Vector.h"
#include "Quaternion.h"
#include "Texture.h"
#include "GameWorldDOD.h"
#include "PhysicsSystemDOD.h"

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
			size_t objectIndex;
			size_t floorIndex;
			float forceMagnitude;
			bool useGravity;

			TutorialGameData()
				:forceMagnitude(10), objectIndex((size_t)-1), floorIndex((size_t)-1) {
			}
		};

		struct TutorialGameRosources {
			Rendering::Mesh* cubeMesh;
			Rendering::Mesh* sphereMesh;

			Rendering::Texture* defaultTex;
			Rendering::Texture* checkerTex;

			TutorialGameRosources()
				:cubeMesh(nullptr), sphereMesh(nullptr), defaultTex(nullptr), checkerTex(nullptr) {
			}
		};

		class TutorialGameDOD {
		public:
			TutorialGameDOD(GameWorldDOD& gameWorld, GameTechRendererInterface& renderer, PhysicsSystemDOD& physics);
			~TutorialGameDOD();

			TutorialGameData data;
			TutorialGameRosources resources;

			void InitWorld();
			void UpdateGame(float dt);
			void initTest();

			void ClearGame();

		private:
			GameWorldDOD& gameWorld;
			GameTechRendererInterface& renderer;
			PhysicsSystemDOD& physics;
			Controller* controller;

			void InitCamera();
			void LoadResources();
			
			size_t AddFloorToWorld(const Vector3& position, float floorHeight, float floorLenght, int collisionLayer = 0);
			size_t addCubeToWorld(const Vector3& position, const Vector3& cubeDims, float inverseMass = 10.0f, int collisionLayer = 0);
			size_t addSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f, int collisionLayer = 0);
			void CreateAABBGrid(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);

			void UpdateCamera(float dt);
			void UpdatePhysics(float dt);

			void HandleInput(float dt);
		};
	}

}