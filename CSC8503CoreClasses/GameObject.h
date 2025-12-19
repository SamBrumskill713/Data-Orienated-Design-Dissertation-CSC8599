#pragma once
#include "Transform.h"
#include "CollisionVolume.h"
#include "GameWorld.h"

using std::vector;

namespace NCL::CSC8503 {
	class NetworkObject;
	class RenderObject;
	class PhysicsObject;
	class playerObject;

	class GameObject {
	public:
		GameObject(const std::string& name = "");
		~GameObject();

		void SetBoundingVolume(CollisionVolume* vol)
		{
			boundingVolume = vol;
		}

		const CollisionVolume* GetBoundingVolume() const
		{
			return boundingVolume;
		}

		bool IsActive() const
		{
			return isActive;
		}

		Transform& GetTransform()
		{
			return transform;
		}

		RenderObject* GetRenderObject() const
		{
			return renderObject;
		}

		PhysicsObject* GetPhysicsObject() const
		{
			return physicsObject;
		}

		NetworkObject* GetNetworkObject() const
		{
			return networkObject;
		}

		void SetNetworkObject(NetworkObject* newObject)
		{
			networkObject = newObject;
		}

		void SetRenderObject(RenderObject* newObject)
		{
			renderObject = newObject;
		}

		void SetPhysicsObject(PhysicsObject* newObject)
		{
			physicsObject = newObject;
		}

		const std::string& GetName() const
		{
			return name;
		}

		virtual void OnCollisionBegin(GameObject* otherObject) {
			//setIsCollided(true);
			/*const CollisionVolume* colInitalObject = GetBoundingVolume();
			const CollisionVolume* colOtherObject = GetBoundingVolume();
			if ((colInitalObject && colOtherObject->isTrigger) || (colInitalObject->isTrigger && colOtherObject)) {
				
			}*/
			//std::cout << "OnCollisionBegin event occured!\n";
		}

		virtual void OnCollisionEnd(GameObject* otherObject) {
			setIsCollided(false);
			//std::cout << "OnCollisionEnd event occured!\n";
		}

		virtual void Update(float dt)
		{

		}

		bool GetBroadphaseAABB(Vector3& outsize) const;

		void UpdateBroadphaseAABB();

		void SetWorldID(int newID)
		{
			worldID = newID;
		}

		int		GetWorldID() const
		{
			return worldID;
		}

		void setIsCollided(bool colliding) {
			isCollided = colliding;
		}

		bool getIsCollided() const {
			return isCollided;
		}

		void setGameWorld(GameWorld* w) {
			w = world;
		}

	protected:
		Transform			transform;

		CollisionVolume* boundingVolume;
		PhysicsObject* physicsObject;
		RenderObject* renderObject;
		NetworkObject* networkObject;
		GameWorld* world = nullptr;

		bool				isActive;
		bool				isCollided = false;
		int					worldID;
		std::string			name;

		Vector3				broadphaseAABB;
	};

	class triggerObject : public GameObject {

	};

	class pickUpObject : public GameObject {
	public:
		pickUpObject(int type);
		~pickUpObject();

		int getPointValue() {
			if (type == defaultType) {
				pointValue = 1;
			}

			else if (type == bonusPointType) {
				pointValue = 5;
			}
			return pointValue;
		}

		void setIsRender(bool ren) {
			isRendered = ren;
		}

		bool getIsRendered(){
			return isRendered;
		}

		Vector4 getColour() {
			return colour;
		}

	protected:
		const int defaultType = 0;
		const int bonusPointType = 1;
		int pointValue;
		int type;
		bool isRendered = true;
		Vector4 colour;
	};

	class playerObject : public GameObject {
	public:
		void setRespawn(Vector3 position);

		void respawn();

		void pickUpItem(pickUpObject* pickup);

		void removeItem();

		void OnCollisionBegin(GameObject* other) override;

		int getpickUpSize() {
			return pickUps.size();
		}

		int getScore() {
			return score;
		}

		void updateItemTransforms(const float dt);

		void Update(float dt) override {
			updateItemTransforms(dt);
		}

	protected:
		bool hasPickup = false;
		Vector3 playerPos;
		int score = 0;
		std::vector<pickUpObject*> pickUps;
	};

	class obstacleObject : public GameObject {
	public:
		void OnCollisionBegin(GameObject* other) override;
	};
}

