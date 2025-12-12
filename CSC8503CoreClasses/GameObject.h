#pragma once
#include "Transform.h"
#include "CollisionVolume.h"

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

	protected:
		Transform			transform;

		CollisionVolume* boundingVolume;
		PhysicsObject* physicsObject;
		RenderObject* renderObject;
		NetworkObject* networkObject;

		bool				isActive;
		bool				isCollided = false;
		int					worldID;
		std::string			name;

		Vector3				broadphaseAABB;
	};

	class triggerObject : public GameObject {

	};

	class pickUpObject : public GameObject {
	};

	class playerObject : public GameObject {
	public:
		void setRespawn(const Vector3& position);

		void pickUpItem(pickUpObject* pickup);

		void removeItem();

		void OnCollisionBegin(GameObject* other) override;

		int getpickUpSize() {
			return pickUps.size();
		}

		void updateItemTransforms(const float dt);

		void Update(float dt) override {
			updateItemTransforms(dt);
		}

	protected:
		bool hasPickup = false;

		Vector3 playerPos;

		std::vector<GameObject*> pickUps;
	};

	class enemyObject : public GameObject {
		
	};

	class obstacleObject : public GameObject {
	public:
		void OnCollisionBegin(GameObject* other) override;
	};

	class movingPlatformObject : public GameObject {

	};
}

