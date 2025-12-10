#pragma once
namespace NCL {
	const int defaultLayer = 0;
	const int playerLayer = 1;
	const int enemyLayer = 2;
	const int pickupLayer = 3;
	const int terrainLayer = 4;

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

	class CollisionVolume
	{
	public:
		CollisionVolume() 
		{
			type = VolumeType::Invalid;
		}
		~CollisionVolume() = default;

		VolumeType type;
		int collisionLayer;
	};
}