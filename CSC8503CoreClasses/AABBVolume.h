#pragma once
#include "CollisionVolume.h"

namespace NCL {
	using namespace NCL::Maths;
	class AABBVolume : public CollisionVolume
	{
	public:
		AABBVolume(const Vector3& halfDims, bool isTrigger = false) {
			type		= VolumeType::AABB;
			halfSizes	= halfDims;
			this->isTrigger = isTrigger;
		}
		~AABBVolume() = default;

		Vector3 GetHalfDimensions() const {
			return halfSizes;
		}

	protected:
		Vector3 halfSizes;
	};
}
