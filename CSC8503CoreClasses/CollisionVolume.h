#pragma once
namespace NCL {
	const int defaultLayer = 0;
	const int playerLayer = 1;
	const int enemyLayer = 2;
	const int pickupLayer = 3;
	const int terrainLayer = 4;
	const int playerColliderLayer = 5;

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

	constexpr int LayerCount = 6;

	constexpr bool CollisionMatrix[LayerCount][LayerCount] = {
		/* defaultLayer(0) */ { true,  true,  true,  true,  true,  true  },
		/* playerLayer (1) */ { true,  true,  true,  true,  true,  false },
		/* enemyLayer  (2) */ { true,  true,  true,  false, true,  false },
		/* pickupLayer (3) */ { true,  true,  false, true,  true,  false },
		/* terrainLayer(4) */ { true,  true,  true,  true,  true,  false },
		/* playerColl(5)  */  { true,  false, false, false, true, false }
	};

	class CollisionVolume
	{
	public:
		CollisionVolume() 
		{
			type = VolumeType::Invalid;
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
	};
}