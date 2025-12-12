#pragma once
#include "RenderObject.h"
#include "../CSC8503CoreClasses/CollisionVolume.h"
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
		class StateGameObject;
		class playerObject;
		class pickUpObject;

		class TutorialGame {
		public:
			TutorialGame(GameWorld& gameWorld, GameTechRendererInterface& renderer, PhysicsSystem& physics);
			~TutorialGame();

			virtual void UpdateGame(float dt);

		protected:
			void InitCamera();

			void attachCameraToPlayer();

			void movePlayerObject(float dt);

			void InitWorld();

			/*
			These are some of the world/object creation functions I created when testing the functionality
			in the module. Feel free to mess around with them to see different objects being created in different
			test scenarios (constraints, collision types, and so on).
			*/
			void InitGameExamples();

			void InitTriggerTest();

			//TODO: When implementing AI, use this to test
			void initAITest();

			//TODO: When implementing a feature, use this to test
			void initObstacleTest();

			void CreateSphereGrid(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void CreatedMixedGrid(int numRows, int numCols, float rowSpacing, float colSpacing);
			void CreateAABBGrid(int numRows, int numCols, float rowSpacing, float colSpacing, const NCL::Maths::Vector3& cubeDims);

			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();

			void BridgeConstraintTest();

			void pendulumConstraint(const Vector3& anchorPos, int numLinks, float linkLength);

			GameObject* AddFloorToWorld(const NCL::Maths::Vector3& position, float floorHeight, float floorLength, 
				bool isTrigger = false, int collisionLayer = terrainLayer);

			GameObject* AddSphereToWorld(const NCL::Maths::Vector3& position, float radius, bool isRendered, 
				float inverseMass = 10.0f, bool isTrigger = false, int collisionLayer = defaultLayer);

			GameObject* AddCubeToWorld(const NCL::Maths::Vector3& position, NCL::Maths::Vector3 dimensions, 
				float inverseMass = 10.0f,bool isTrigger = false, int collisionLayer = defaultLayer);

			playerObject* AddPlayerToWorld(const NCL::Maths::Vector3& position, Rendering::Mesh* characterMesh,
			const float scale, bool isTrigger = false, int collisionLayer = playerLayer);

			GameObject* AddEnemyToWorld(const NCL::Maths::Vector3& position, Rendering::Mesh* characterMesh,
				const float scale, bool isTrigger = false, int collisionLayer = enemyLayer);

			GameObject* AddBonusToWorld(const NCL::Maths::Vector3& position, Rendering::Mesh* characterMesh,
				const float scale, bool isTrigger = false);

			pickUpObject* AddPickupToWorld(const NCL::Maths::Vector3& position, Rendering::Mesh* pickupMesh,
				const float scale, int pointvalue, bool isTrigger = true, int collisionLayer = pickupLayer);

			//playerObject* initalisePlayerObject()
			StateGameObject* AddStateObjectToWorld(const NCL::Maths::Vector3& position, Rendering::Mesh* characterMesh,
				const float scale);
			//GameObject* setLockedObject();

			GameWorld& world;
			GameTechRendererInterface& renderer;
			PhysicsSystem& physics;
			Controller* controller;

			bool useGravity;
			bool inSelectionMode;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;

			Rendering::Mesh* capsuleMesh	= nullptr;
			Rendering::Mesh* cubeMesh		= nullptr;
			Rendering::Mesh* sphereMesh		= nullptr;

			Rendering::Texture* defaultTex  = nullptr;
			Rendering::Texture* checkerTex	= nullptr;
			Rendering::Texture* glassTex	= nullptr;

			//Coursework Meshes
			Rendering::Mesh* catMesh	= nullptr;
			Rendering::Mesh* kittenMesh = nullptr;
			Rendering::Mesh* enemyMesh	= nullptr;
			Rendering::Mesh* bonusMesh	= nullptr;

			GameTechMaterial checkerMaterial;
			GameTechMaterial glassMaterial;
			GameTechMaterial notexMaterial;

			playerObject* playerObj = nullptr;
			Quaternion* playerOrientation;
			GameObject* playerGroundCollision = nullptr;
			pickUpObject* testTrigger = nullptr;
			pickUpObject* testTrigger2 = nullptr;


			//Coursework Additional functionality	
			GameObject* lockedObject = nullptr;
			NCL::Maths::Vector3 lockedOffset = NCL::Maths::Vector3(0, 14, 20);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}

			GameObject* objClosest = nullptr;
			StateGameObject* testStateObject = nullptr;
		};	
	}
}