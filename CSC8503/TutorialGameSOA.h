#pragma once
#include "Vector.h"
#include "Quaternion.h"
#include "Texture.h"
#include "GameWorldSOA.h"
#include "PhysicsSystemSOA.h"
#include "GameTechRendererDOD.h"
#include "GameTechRendererSOA.h"

namespace NCL {
	class Controller;

	namespace Rendering {
		class Mesh;
		class Texture;
		class Shader;
	}

	namespace CSC8503 {
		class GameTechRendererInterface;

		struct TutorialGameDataSOA {
			size_t objectIndex;
			size_t floorIndex;
			float forceMagnitude;
			bool useGravity;
			int x;
			int y;

			TutorialGameDataSOA()
				:forceMagnitude(10), objectIndex((size_t)-1), floorIndex((size_t)-1) {
			}
		};

		struct TutorialGameResourcesSOA {
			Rendering::Mesh* cubeMesh;
			Rendering::Mesh* sphereMesh;

			Rendering::Texture* defaultTex;
			Rendering::Texture* checkerTex;

			GameTechMaterial checkerMaterial;

			TutorialGameResourcesSOA()
				:cubeMesh(nullptr), sphereMesh(nullptr), defaultTex(nullptr), checkerTex(nullptr) {
			}
		};

		class TutorialGameSOA {
		public:
			TutorialGameSOA(GameWorldSOA& gameWorld, RendererSystemSOA& renderer, PhysicsSystemSOA& physics);
			~TutorialGameSOA();

			TutorialGameDataSOA data;
			TutorialGameResourcesSOA resources;

			void InitWorld();
			void UpdateGame(float dt);
			void InitTest();

		private:
			GameWorldSOA& gameWorld;
			//RendererSystemDOD& rendererDOD;
			RendererSystemSOA& rendererSOA;
			PhysicsSystemSOA& physics;
			Controller* controller;

			void InitCamera();
			void LoadResources();

			int AddFloorToWorld(const Vector3& position, float floorHeight, float floorLength, int collisionLayer = 0);
			int AddCubeToWorld(const Vector3& position, const Vector3& cubeDims, float inverseMass = 10.0f, int collisionLayer = 0);
			void CreateAABBGrid(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
		};
	}
}