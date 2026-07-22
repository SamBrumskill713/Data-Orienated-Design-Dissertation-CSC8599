#pragma once

#include <cstdint>
#include "Matrix.h"
#include "Maths.h"
#include <vector>
#include "CollisionVolumeDOD.h"

using namespace NCL::Maths;

namespace NCL::CSC8503 {

	enum class VolumeTypeSOA {
		AABB = 1,
		OBB = 2,
		Sphere = 4,
		Mesh = 8,
		Capsule = 16,
		Compound = 32,
		Invalid = 256
	};

	struct CollisionVolumeCompSOA {
		std::vector<VolumeTypeSOA> typeSOA;
		std::vector<int> collisionLayerSOA;
		std::vector<char> isTriggerSOA;
	};

	struct AABBCompSOA {
		std::vector<Vector3> halfSizesSOA;
	};

	struct CollisionVolumeSysSOA {
		AABBCompSOA AABBDataSOA;
		CollisionVolumeCompSOA dataSOA;
		std::vector<int> typeDataIndex;

		int CreateAABB(const Vector3& halfSizes, int layer, bool trigger = false) {
			int volumeIndex = dataSOA.typeSOA.size();

			dataSOA.typeSOA.emplace_back(VolumeTypeSOA::AABB);
			dataSOA.collisionLayerSOA.emplace_back(layer);
			dataSOA.isTriggerSOA.emplace_back(trigger);

			AABBDataSOA.halfSizesSOA.emplace_back(halfSizes);
			typeDataIndex.emplace_back((int)AABBDataSOA.halfSizesSOA.size() - 1);

			return volumeIndex;
		}

		void RemoveCollisionVolume(int index) {
			if (index < 0 || index >= (int)dataSOA.typeSOA.size()) return;

			int lastIndex = dataSOA.typeSOA.size() - 1;

			int typeIndex = typeDataIndex[index];
			int lastTypeIndex = typeDataIndex[lastIndex];

			if (index != lastIndex) {
				dataSOA.typeSOA[index] = dataSOA.typeSOA[lastIndex];
				dataSOA.collisionLayerSOA[index] = dataSOA.collisionLayerSOA[lastIndex];
				dataSOA.isTriggerSOA[index] = dataSOA.isTriggerSOA[lastIndex];

				AABBDataSOA.halfSizesSOA[typeIndex] = AABBDataSOA.halfSizesSOA[lastTypeIndex];

				typeDataIndex[index] = typeIndex;
			}

			dataSOA.typeSOA.pop_back();
			dataSOA.collisionLayerSOA.pop_back();
			dataSOA.isTriggerSOA.pop_back();

			AABBDataSOA.halfSizesSOA.pop_back();
			typeDataIndex.pop_back();
		}

		int GetCollisionCount() const {
			return dataSOA.typeSOA.size();
		}
	};
}