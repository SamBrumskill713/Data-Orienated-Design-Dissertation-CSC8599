#pragma once
#include <vector>
#include "Vector.h"
#include "Quaternion.h"
#include "Matrix.h"
#include "Plane.h"
#include "Ray.h"
#include "Camera.h"
#include "TransformDOD.h"
#include "CollisionVolumeDOD.h"
#include "GameObjectDOD.h"

using namespace NCL::Maths;

namespace NCL::CSC8503 {

	// Contact info for a collision between two objects
	struct CollisionInfoDOD {
		size_t entityA;
		size_t entityB;
		int framesLeft;

		Vector3 localA;
		Vector3 localB;
		Vector3 normal;
		float penetration;
		bool isActive;

		CollisionInfoDOD()
			: entityA(0), entityB(0), framesLeft(0), normal(Vector3()),
			penetration(0.0f), isActive(false) {
		}

		CollisionInfoDOD(size_t a, size_t b)
			: entityA(a), entityB(b), framesLeft(0), normal(Vector3()),
			penetration(0.0f), isActive(false) {
		}

		void AddContactPoint(const Vector3& lA, const Vector3& lB, const Vector3& norm, float pene) {
			localA = lA;
			localB = lB;
			normal = norm;
			penetration = pene;
		}
	};

	// Raycast result
	struct RayCollisionDOD {
		void* node;
		Vector3 collidedAt;
		float rayDistance;
		bool hasCollided;

		RayCollisionDOD()
			: node(nullptr), collidedAt(Vector3()),
			rayDistance(FLT_MAX), hasCollided(false) {
		}
	};

	// Simplified collision detection system - AABB only
	class CollisionDetectionDOD {
	public:
		// Object-to-object AABB collision detection
		static bool ObjectIntersection(const GameObjectDOD& objA, const GameObjectDOD& objB,
			size_t idxA, size_t idxB, CollisionInfoDOD& collisionInfo);

		// Batch collision detection
		static void DetectAllCollisions(GameObjectStorage& storage, std::vector<CollisionInfoDOD>& outCollisions);

		// Raycast operations
		static bool RayPlaneIntersection(const Ray& r, const Plane& p, RayCollisionDOD& collision);
		static bool RayBoxIntersection(const Ray& r, const Vector3& boxPos, const Vector3& boxSize, RayCollisionDOD& collision);
		static bool RayAABBIntersection(const Ray& r, const TransformsComp& transform, const AABBComp& volume, RayCollisionDOD& collision);

		// AABB primitive test
		static bool AABBTest(const Vector3& posA, const Vector3& posB, const Vector3& halfSizeA, const Vector3& halfSizeB);

		// AABB collision detection
		static bool AABBIntersection(const TransformsComp& transformA, const AABBComp& volumeA,
			const TransformsComp& transformB, const AABBComp& volumeB, CollisionInfoDOD& collisionInfo);

		// Utility
		static Vector3 Unproject(const Vector3& screenPos, const PerspectiveCamera& cam);
		static Ray BuildRayFromMouse(const PerspectiveCamera& cam);

	private:
		CollisionDetectionDOD() {}
		~CollisionDetectionDOD() {}

		static Matrix4 GenerateInverseView(const Camera& c);
		static Matrix4 GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane);
	};
}