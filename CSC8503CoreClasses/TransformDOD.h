#pragma once
#include "Vector.h"
#include "Quaternion.h"
#include "Matrix.h"

using namespace NCL::Maths;
namespace NCL {
	namespace CSC8503 {
		struct TransformsComp {
			Matrix4 matrix;
			Quaternion orientation;
			Vector3 position;
			Vector3 scale = Vector3(1, 1, 1);
		};

		struct TransformsSys {
			TransformsComp transformData;

			TransformsComp& SetPosition(const Vector3& worldPos);
			TransformsComp& SetScale(const Vector3& worldScale);
			TransformsComp& SetOrientation(const Quaternion& newOr);

			Vector3 GetPosition() {
				return transformData.position;
			}

			Quaternion GetOrientation() {
				return transformData.orientation;
			}

			Matrix4 GetMatrix() {
				return transformData.matrix;
			}
		private:
			void UpdateMatrix();
		};
	}
}