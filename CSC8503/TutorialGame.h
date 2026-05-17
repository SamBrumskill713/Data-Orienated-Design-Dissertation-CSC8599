#pragma once
#include "RenderObject.h"
#include "../CSC8503CoreClasses/CollisionVolume.h"
#include "../CSC8503CoreClasses/GameObject.h"
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
		class obstacleObject;
		class triggerObject;
		class EnemyObject;
		class levelElements;

		class TutorialGame {
		public:
			TutorialGame(GameWorld& gameWorld, GameTechRendererInterface& renderer, PhysicsSystem& physics);
			~TutorialGame();
			void InitWorld();
			virtual void UpdateGame(float dt);
			bool IsGameOver() const { return isGameOver; }
			bool IsWin() const { return isWin; }
			int  GetPlayerScore() const; 

			void ClearEndState() {
				isGameOver = false;
				isWin = false;
				InitWorld();
			}

			void SetLocalPlayerControl(bool enabled) {
				allowLocalPlayerControl = enabled;
			}

			bool IsLocalPlayerControlEnabled() const {
				return allowLocalPlayerControl;
			}

			void SetCameraTarget(GameObject* target) {
				cameraTarget = target;
			}

			Quaternion* getPlayerOrientaiton(Quaternion* playOr) {
				playerOrientation = playOr;
			}

		protected:
			void InitCamera();

			void attachCameraToPlayer();

			void movePlayerObject(float dt);

			/*
			These are some of the world/object creation functions I created when testing the functionality
			in the module. Feel free to mess around with them to see different objects being created in different
			test scenarios (constraints, collision types, and so on).
			*/
			void InitGameExamples();

			void InitTriggerTest();

			void initAITest();

			void initGame();

			void initFPSTest();

			void initObstacleTest();

			void CreateSphereGrid(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void CreatedMixedGrid(int numRows, int numCols, float rowSpacing, float colSpacing);
			void CreateAABBGrid(int numRows, int numCols, float rowSpacing, float colSpacing, const NCL::Maths::Vector3& cubeDims);

			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();

			void BridgeConstraintTest();

			virtual void OnEnemySpawned(EnemyObject& enemy) {

			}

			obstacleObject* pendulumConstraint(const Vector3& anchorPos, int numLinks, float linkLength);

			levelElements* levelCreate();

			GameObject* AddFloorToWorld(const NCL::Maths::Vector3& position, float floorHeight, float floorLength, 
				bool isTrigger = false, int collisionLayer = terrainLayer);

			GameObject* AddSphereToWorld(const NCL::Maths::Vector3& position, float radius, bool isRendered, 
				float inverseMass = 10.0f, bool isTrigger = false, int collisionLayer = defaultLayer);

			GameObject* AddCubeToWorld(const NCL::Maths::Vector3& position, NCL::Maths::Vector3 dimensions, 
				float inverseMass = 10.0f,bool isTrigger = false, int collisionLayer = defaultLayer);

			playerObject* AddPlayerToWorld(const NCL::Maths::Vector3& position, Rendering::Mesh* characterMesh,
			const float scale, bool isTrigger = false, int collisionLayer = playerLayer);

			EnemyObject* AddEnemyToWorld(const NCL::Maths::Vector3& position, Rendering::Mesh* characterMesh,
				const float scale, bool isTrigger = false, int collisionLayer = enemyLayer);

			GameObject* AddBonusToWorld(const NCL::Maths::Vector3& position, Rendering::Mesh* characterMesh,
				const float scale, bool isTrigger = false, int collisionLayer = defaultLayer);

			pickUpObject* AddPickupToWorld(const NCL::Maths::Vector3& position, Rendering::Mesh* pickupMesh,
				const float scale, int type = 0, bool isTrigger = true, int collisionLayer = pickupLayer);

			triggerObject* addTriggerVolume(const NCL::Maths::Vector3& position, const float scaleX,
				const float scaleY, const float scaleZ, const Vector3& trigHalfDims, int collisionLayer = triggerVolume);

			GameObject* addWall(const NCL::Maths::Vector3& position, Vector3& halfDims, float inverseMass = 0, 
				int collisionLayer = defaultLayer);

			GameObject* addDropOffZone(const NCL::Maths::Vector3& position, Vector3& halfDims, bool isTrigger = true,
				int collisionLayer = dropZoneLayer);

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
			Rendering::Mesh* goatMesh   = nullptr;
			Rendering::Mesh* playerMesh = nullptr;

			GameTechMaterial checkerMaterial;
			GameTechMaterial glassMaterial;
			GameTechMaterial notexMaterial;

			playerObject* playerObj = nullptr;
			Quaternion* playerOrientation;
			GameObject* playerGroundCollision = nullptr;
			pickUpObject* pickUp = nullptr;
			triggerObject* trigVol = nullptr;
			triggerObject* outOfBounds = nullptr;
			obstacleObject* pendulum = nullptr;
			EnemyObject* enemyAI = nullptr;
			levelElements* data;
			GameObject* movableWall = nullptr;
			GameObject* dropOffZone = nullptr;
			GameObject* levelFloor = nullptr;
			float gameTime;
			bool isGameOver = false;
			bool isWin = false;
			bool allowLocalPlayerControl = true;
			GameObject* cameraTarget = nullptr;
			std::vector<pickUpObject*> levelItems;
			std::vector<EnemyObject*> enemies;
			std::vector<obstacleObject*> obstacles;

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