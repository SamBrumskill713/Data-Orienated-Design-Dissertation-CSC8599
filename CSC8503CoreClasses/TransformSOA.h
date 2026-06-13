#pragma once

#include "Vector.h"
#include "Quaternion.h"
#include "Matrix.h"
#include <vector>

using namespace NCL::Maths;

namespace NCL::CSC8503 {
	struct TransformCompSOA {
		std::vector<Matrix4> matrices;
		std::vector<Quaternion>orientations;
		std::vector<Vector3> positions;
		std::vector<Vector3> scales;
	};

	namespace TransformOpsSOA {
		inline int AddTransform(TransformCompSOA& Transforms,
			const Vector3& Position = Vector3(0, 0, 0),
			const Quaternion& Orientation = Quaternion(),
			const Vector3& scale = Vector3(1, 1, 1)) {
			int index = Transforms.positions.size();
			Transforms.positions.push_back(Position);
			Transforms.orientations.push_back(Orientation);
			Transforms.scales.push_back(scale);
			Transforms.matrices.push_back(Matrix4());
			return index;
		}

		inline void RemoveTransform(TransformCompSOA& Transforms, int index) {
			if (index < 0 || index >= (int)Transforms.positions.size()) return;

			int lastIndex = Transforms.positions.size() - 1;
			if (index != lastIndex) {
				Transforms.positions[index] = Transforms.positions[lastIndex];
				Transforms.orientations[index] = Transforms.orientations[lastIndex];
				Transforms.scales[index] = Transforms.scales[lastIndex];
				Transforms.matrices[index] = Transforms.matrices[lastIndex];
			}

			Transforms.positions.pop_back();
			Transforms.orientations.pop_back();
			Transforms.scales.pop_back();
			Transforms.matrices.pop_back();
		}

		inline int GetCount(const TransformCompSOA& Transfroms) {
			return Transfroms.positions.size();
		}

		inline void UpdateMatrixSOA(TransformCompSOA& Transforms, int index) {
			Transforms.matrices[index] = Matrix::Translation(Transforms.positions[index]) *
				Quaternion::RotationMatrix<Matrix4>(Transforms.orientations[index]) *
				Matrix::Scale(Transforms.scales[index]);
		}

		inline void SetPositionSOA(TransformCompSOA& Transforms, const Vector3& worldPos, int index) {
			Transforms.positions[index] = worldPos;
			UpdateMatrixSOA(Transforms, index);
		}

		inline void SetScaleSOA(TransformCompSOA& Transforms, const Vector3& worldScale, int index) {
			Transforms.scales[index] = worldScale;
			UpdateMatrixSOA(Transforms, index);
		}

		inline void SetOrientationSOA(TransformCompSOA& Transforms, const Quaternion& worldOrientations, int index) {
			Transforms.orientations[index] = worldOrientations;
			UpdateMatrixSOA(Transforms, index);
		}

		inline void UpdateAllMatrices(TransformCompSOA Transforms) {
			int count = GetCount(Transforms);
			for (size_t i = 0; i < count; ++i) {
				UpdateMatrixSOA(Transforms, i);
			}
		}

		inline void SetAllPositions(TransformCompSOA& Transforms, const std::vector<int>& indices, const std::vector<Vector3>& Positions) {
			for (size_t i = 0; i < indices.size(); ++i) {
				SetScaleSOA(Transforms, Positions[i], indices[i]);
			}
		}

		inline void SetAllScales(TransformCompSOA& Transforms, const std::vector<int>& indices, const std::vector<Vector3>& Scales) {
			for (size_t i = 0; i < indices.size(); ++i) {
				SetScaleSOA(Transforms, Scales[i], indices[i]);
			}
		}

		inline void SetAllOrientations(TransformCompSOA& Transforms, const std::vector<int>& indices, const std::vector<Quaternion>& Orientations) {
			for (size_t i = 0; i < indices.size(); ++i) {
				SetOrientationSOA(Transforms, Orientations[i], indices[i]);
			}
		}
	}
}