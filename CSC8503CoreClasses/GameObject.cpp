#include "GameObject.h"
#include "CollisionDetection.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "NetworkObject.h"

using namespace NCL::CSC8503;

GameObject::GameObject(const std::string& objectName)	
{
	name			= objectName;
	worldID			= -1;
	isActive		= true;
	boundingVolume	= nullptr;
	physicsObject	= nullptr;
	renderObject	= nullptr;
	networkObject	= nullptr;
}

GameObject::~GameObject()	
{
	delete boundingVolume;
	delete physicsObject;
	delete renderObject;
	delete networkObject;
}

bool GameObject::GetBroadphaseAABB(Vector3&outSize) const 
{
	if (!boundingVolume) {
		return false;
	}
	outSize = broadphaseAABB;
	return true;
}

void GameObject::UpdateBroadphaseAABB() 
{
	if (!boundingVolume) {
		return;
	}
	switch (boundingVolume->type)
	{
		case VolumeType::AABB : {
			broadphaseAABB = ((AABBVolume&)*boundingVolume).GetHalfDimensions();
		}break;
		case VolumeType::Sphere: {
			float r = ((SphereVolume&)*boundingVolume).GetRadius();
			broadphaseAABB = Vector3(r, r, r);
		}break;
		case VolumeType::OBB: {
			Matrix3 mat = Quaternion::RotationMatrix<Matrix3>(transform.GetOrientation());
			mat = Matrix::Absolute(mat);
			Vector3 halfSizes = ((OBBVolume&)*boundingVolume).GetHalfDimensions();
			broadphaseAABB = mat * halfSizes;
		}break;
		default: {
			std::cout << "Object " << this->name << " has unsupported bounding volume type for GameObject::UpdateBroadphaseAABB()\n";
		}
	}
}

void NCL::CSC8503::playerObject::setRespawn(const Vector3& position)
{
	playerPos = position;
	GetTransform().SetPosition(playerPos);
	if (auto* phys = GetPhysicsObject()) {
		phys->ClearForces();
		phys->SetLinearVelocity(Vector3(0, 0, 0));
		phys->SetAngularVelocity(Vector3(0, 0, 0));
	}
}

void NCL::CSC8503::playerObject::pickUpItem(pickUpObject* pickup)
{
	pickUps.emplace_back(pickup);

	// Make picked item non-blocking and move it to inventory layer
	if (auto* vol = const_cast<CollisionVolume*>(pickup->GetBoundingVolume())) {
		vol->isTrigger = true;
		vol->collisionLayer = NCL::itemInventoryLayer; 
	}
}

void NCL::CSC8503::playerObject::OnCollisionBegin(GameObject* other)
{
	if (other->GetBoundingVolume()->collisionLayer == NCL::pickupLayer) {
		if (auto* itemPickUp = dynamic_cast<pickUpObject*>(other)) {
			pickUpItem(itemPickUp);
			itemPickUp->setIsCollided(false);
		}
	}
}

void NCL::CSC8503::playerObject::updateItemTransforms(const float dt)
{
	if (pickUps.empty()) {
		hasPickup = false;
		return;
	}
	hasPickup = true;

	const float baseHeightOffset = 4.0f;
	const float itemSpacing = 1.0f;
	const Vector3 playerPos = this->GetTransform().GetPosition();

	for (size_t i = 0; i < pickUps.size(); ++i) {
		const float yOffset = baseHeightOffset + (itemSpacing * static_cast<float>(i));
		const Vector3 targetPos = playerPos + Vector3(0.0f, yOffset, 0.0f);

		if (auto* phys = pickUps[i]->GetPhysicsObject()) {
			phys->ClearForces();
			phys->SetLinearVelocity(Vector3(0, 0, 0));
			phys->SetAngularVelocity(Vector3(0, 0, 0));
		}

		pickUps[i]->GetTransform().SetPosition(targetPos);
		pickUps[i]->UpdateBroadphaseAABB();
	}
}
