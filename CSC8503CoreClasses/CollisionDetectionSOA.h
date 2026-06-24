#pragma once

#include <vector>
#include "Vector.h"
#include "Quaternion.h"
#include "Matrix.h"
#include "Plane.h"
#include "Ray.h"
#include "Camera.h"
#include "TransformSOA.h"
#include "CollisionVolumeSOA.h"
#include "GameObjectSOA.h"

using namespace NCL::Maths;

namespace NCL::CSC8503 {

	struct CollisionInfoSOA {
		int entityA;
		int entityB;
		int framesLeft;

		Vector3 localA;
		Vector3 localB;
		Vector3 normal;
		float penetration;
		bool isActive;

		CollisionInfoSOA();
		CollisionInfoSOA(int a, int b);

		void AddContactPoint(const Vector3& lA, const Vector3& lB, const Vector3& norm, float pene);
	};

	struct RayCollisionSOA {
		void* node;
		Vector3 collidedAt;
		float rayDistance;
		bool hasCollided;

		RayCollisionSOA();
	};

	class CollisionDetectionSOA {
	public:
		static bool ObjectIntersection(const GameObjectCompSOA& gameObjects,
			int idxA, int idxB, CollisionInfoSOA& collisionInfo);

		static void DetectAllCollisions(GameObjectCompSOA& gameObjects,
			std::vector<CollisionInfoSOA>& outCollisions);

		static bool RayPlaneIntersection(const Ray& r, const Plane& p, RayCollisionSOA& collision);
		static bool RayBoxIntersection(const Ray& r, const Vector3& boxPos, const Vector3& boxSize, RayCollisionSOA& collision);
		static bool RayAABBIntersection(const Ray& r, int objectIndex, const GameObjectCompSOA& gameObjects, RayCollisionSOA& collision);

		static bool AABBTest(const Vector3& posA, const Vector3& posB,
			const Vector3& halfSizeA, const Vector3& halfSizeB);

		static bool AABBIntersection(int indexA, int indexB,
			const GameObjectCompSOA& gameObjects, CollisionInfoSOA& collisionInfo);

		static Vector3 Unproject(const Vector3& screenPos, const PerspectiveCamera& cam);
		static Ray BuildRayFromMouse(const PerspectiveCamera& cam);

	private:
		CollisionDetectionSOA() {}
		~CollisionDetectionSOA() {}

		static Matrix4 GenerateInverseView(const Camera& c);
		static Matrix4 GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane);
	};
}