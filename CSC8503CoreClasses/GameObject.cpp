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

void NCL::CSC8503::playerObject::setRespawn(Vector3 position)
{
	playerPos = position;
	GetTransform().SetPosition(playerPos);
	if (auto* phys = GetPhysicsObject()) {
		phys->ClearForces();
		phys->SetLinearVelocity(Vector3(0, 0, 0));
		phys->SetAngularVelocity(Vector3(0, 0, 0));
	}
}

void NCL::CSC8503::playerObject::respawn()
{
	this->GetTransform().SetPosition(playerPos);
}

void NCL::CSC8503::playerObject::pickUpItem(pickUpObject* pickup)
{
	hasPickup = true;
	pickUps.emplace_back(pickup);

	if (auto* vol = const_cast<CollisionVolume*>(pickup->GetBoundingVolume())) {
		vol->isTrigger = true;
		vol->collisionLayer = NCL::itemInventoryLayer; 
	}
}

void NCL::CSC8503::playerObject::removeItem()
{
	if (pickUps.empty()) {
		hasPickup = false;
		return;
	}

	pickUpObject* droppedItem = pickUps.back();

	pickUps.pop_back();

	Vector3 vel = Vector3(0, 0, 0);

	if (auto* droppedPhysics = droppedItem->GetPhysicsObject()) {
		vel = droppedPhysics->GetLinearVelocity();
	}

	Vector3 backDir = Vector::Length(vel) > 0.001f ? Vector::Normalise(vel) : Vector3(0, 0, 1);

	const float dropDistance = 5.0f;
	const float dropImpulse = 10.0f;

	Vector3 playerPos = GetTransform().GetPosition();
	Vector3 dropPos = playerPos + backDir * dropDistance;
	dropPos.y = playerPos.y;

	droppedItem->GetTransform().SetPosition(dropPos);

	if (const CollisionVolume* droppedVol = droppedItem->GetBoundingVolume()) {
		auto* vol = const_cast<CollisionVolume*>(droppedVol);
		vol->collisionLayer = pickupLayer;
	}

	if (auto* itemPhys = droppedItem->GetPhysicsObject()) {
		itemPhys->SetLinearVelocity(Vector3(0, 0, 0));
		itemPhys->ClearForces();
		itemPhys->ApplyLinearImpulse(backDir * dropImpulse);
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
	if (other->GetBoundingVolume()->collisionLayer == NCL::dropZoneLayer && !pickUps.empty()) {
		for (int i = 0; i < pickUps.size(); ++i) {
			pickUps[i]->setIsRender(false);
			score += pickUps[i]->getPointValue();
		}
		pickUps.clear();
		hasPickup = false;
		//pickUps.clear();
	}

	if (other->GetBoundingVolume()->collisionLayer == NCL::enemyLayer) {
		std::cout << "player side\n";
		if (!pickUps.empty()) {
			removeItem();
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

void NCL::CSC8503::obstacleObject::OnCollisionBegin(GameObject* other)
{
	if (!other || !other->GetBoundingVolume()) {
		return;
	}

	if (other->GetBoundingVolume()->collisionLayer == triggerVolume) {
		auto* physObj = this->GetPhysicsObject();
		if (physObj) {
			physObj->ApplyLinearImpulse(Vector3(0, -100.0f, 0));
		}
	}
}

NCL::CSC8503::pickUpObject::pickUpObject(int type)
{
	if (type == 0) {
		type = defaultType;
		colour = Vector4(1, 0, 0, 1);
		this->type = type;
	}

	else if (type == 1) {
		type = bonusPointType;
		colour = Vector4(0, 0, 1, 1);
		this->type = type;
	}
}

NCL::CSC8503::pickUpObject::~pickUpObject()
{
}
