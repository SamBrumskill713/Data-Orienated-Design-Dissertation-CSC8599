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

void NCL::CSC8503::playerObject::pickUpItem(GameObject* pickup)
{
	pickUps.emplace_back(pickup);
}

void NCL::CSC8503::pickUpObject::OnCollisionBegin(GameObject* other)
{
	if (other->GetBoundingVolume()->collisionLayer == NCL::playerLayer) {
		if (auto* player = dynamic_cast<playerObject*>(other)) {
			player->setRespawn(Vector3(5, -11.5, 0));
			std::cout << "collision \n";
		}
		//std::cout << "collision \n";
	}
}
