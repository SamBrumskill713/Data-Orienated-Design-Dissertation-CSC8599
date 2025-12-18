#pragma once
namespace NCL {
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

	enum class VolumeType 
	{
		AABB	= 1,
		OBB		= 2,
		Sphere	= 4, 
		Mesh	= 8,
		Capsule = 16,
		Compound= 32,
		Invalid = 256
	};

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

	class CollisionVolume
	{
	public:
		CollisionVolume() 
		{
			type = VolumeType::Invalid;
			isTrigger = false;
		}
		~CollisionVolume() = default;

		static bool layerMask(int a, int b) {
			if (a < 0 || a >= LayerCount || b < 0 || b >= LayerCount) {
				return false;
			}
			return CollisionMatrix[a][b] || CollisionMatrix[b][a];
		}

		VolumeType type;
		int collisionLayer;
		bool isTrigger;
	};
}