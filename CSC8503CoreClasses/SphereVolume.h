#pragma once
#include "CollisionVolume.h"

namespace NCL {
	class SphereVolume : public CollisionVolume
	{
	public:
		SphereVolume(bool isTrigger, float sphereRadius = 1.0f) {
			type	= VolumeType::Sphere;
			radius	= sphereRadius;
			this->isTrigger = isTrigger;
		}
		~SphereVolume() = default;

		float GetRadius() const 
		{
			return radius;
		}
	protected:
		float	radius;
	};
}

