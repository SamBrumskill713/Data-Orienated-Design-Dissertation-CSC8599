#pragma once

#include "Vector.h"
#include "Quaternion.h"
#include "Matrix.h"

using namespace NCL::Maths;

namespace NCL::CSC8503 {

	// Pure data component - transform state
	struct TransformsComp {
		Matrix4 matrix;
		Quaternion orientation;
		Vector3 position;
		Vector3 scale;

		TransformsComp()
			: matrix(Matrix4()), orientation(Quaternion()),
			position(Vector3()), scale(Vector3(1, 1, 1)) {
		}
	};

	namespace TransformOps {

		inline void UpdateMatrix(TransformsComp& transform) {
			transform.matrix = Matrix::Translation(transform.position) *
				Quaternion::RotationMatrix<Matrix4>(transform.orientation) *
				Matrix::Scale(transform.scale);
		}

		inline void SetPosition(TransformsComp& transform, const Vector3& worldPos) {
			transform.position = worldPos;
			UpdateMatrix(transform);
		}

		inline void SetScale(TransformsComp& transform, const Vector3& worldScale) {
			transform.scale = worldScale;
			UpdateMatrix(transform);
		}

		inline void SetOrientation(TransformsComp& transform, const Quaternion& newOr) {
			transform.orientation = newOr;
			UpdateMatrix(transform);
		}
	}
}