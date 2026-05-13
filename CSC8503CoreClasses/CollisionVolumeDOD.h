#pragma once
#include <cstdint>
#include "Matrix.h"
#include "Maths.h"
namespace NCL {
	using namespace NCL::Maths;

	const int defaultLayer = 0;
	const int playerLayer = 1;
	const int enemyLayer = 2;
	const int pickupLayer = 3;
	const int terrainLayer = 4;
	const int playerColliderLayer = 5;
	const int itemInventoryLayer = 6;
	const int triggerVolume = 7;
	const int dropZoneLayer = 8;
	const int outOfBoundLayer = 9;

	constexpr int LayerCount = 10;

	constexpr bool CollisionMatrix[LayerCount][LayerCount] = {
		/* default(0) */ { true,  true,  true,  true,  true,  true,  false, false, false, true },
		/* player (1) */ { true,  true,  true,  true,  true,  false, false, true,  true,  true },
		/* enemy  (2) */ { true,  true,  false, false, true,  false, false, true,  false, true },
		/* pickup (3) */ { true,  true,  false, true,  true,  false, false, false, true,  true },
		/* terrain(4) */ { true,  true,  true,  true,  true,  false, false, false, true,  false},
		/* playerC(5) */ { true,  false, false, false, true,  false, false, false, true,  true },
		/* itemInv(6) */ { false, false, false, false, false, false, false, false, true,  true },
		/* trigger(7) */ { true,  true,  false, false, false, false, false, false, false, false},
		/* dropZone(8) */{ true,  true,  false, false, false, false, false, false, true,  false},
		/* outOfBounds(9) */{ true,  true,  true,  true,  false, true,  false, false, false, true },
	};

	enum class VolumeType {
		AABB = 1,
		OBB = 2,
		Sphere = 4,
		Mesh = 8,
		Capsule = 16,
		Compound = 32,
		Invalid = 256
	};

	struct CollisionVolumeComp {
		Matrix3 orientation;
		Vector3 halfExtents;
		Vector3 halfSizes;
		VolumeType type;
		float halfHeight;
		float radius;
		int collisionLayer;
		bool isTrigger;
	};

	struct CollisionVolumeSys {
		CollisionVolumeComp data;

		CollisionVolumeSys CreateAABB(const Vector3& halfSizes, int layer, bool trigger = false) {
			data.type = VolumeType::AABB;
			data.collisionLayer = layer;
			data.isTrigger = trigger;
			data.halfSizes = halfSizes;
		}

		CollisionVolumeSys CreateSphere(float radius, int layer, bool trigger = false) {
			data.type = VolumeType::Sphere;
			data.collisionLayer = layer;
			data.radius = radius;
			data.isTrigger = trigger;
		}

		CollisionVolumeSys CreateOBB(const Vector3& halfExtents, int layer, bool trigger = false) {
			data.type = VolumeType::OBB;
			data.collisionLayer = layer;
			data.halfExtents = halfExtents;
			data.isTrigger = false;
		}

		CollisionVolumeSys CreateCapsule(float radius, float halfHeight, int layer, bool trigger = false) {
			data.type = VolumeType::Capsule;
			data.collisionLayer = layer;
			data.radius = radius;
			data.halfHeight = halfHeight;
			data.isTrigger = trigger;
		}

		bool isValid() const {
			data.type != VolumeType::Invalid;
		}

		bool CanCollideWith(const CollisionVolumeComp& other) const {
			if (data.collisionLayer < 0 || data.collisionLayer >= LayerCount ||
				other.collisionLayer < 0 || other.collisionLayer >= LayerCount) {
				return false;
			}
			return CollisionMatrix[data.collisionLayer][other.collisionLayer] ||
				CollisionMatrix[other.collisionLayer][data.collisionLayer];
		}

		bool isType(VolumeType type) {
			return data.type == type;
		}

		const Vector3& GetAABBHalfSize() {
			return data.halfSizes;
		}

		float GetSphereRadius() {
			return data.radius;
		}

		const Vector3& GetOBBHalfExtents() {
			return data.halfExtents;
		}

		float GetCapsuleRadius() {
			return data.radius;
		}

		float GetCapsuleHalfHeight() {
			return data.halfHeight;
		}
	};
}