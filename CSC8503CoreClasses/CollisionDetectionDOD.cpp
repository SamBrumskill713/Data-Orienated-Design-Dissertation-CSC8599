#include "CollisionDetectionDOD.h"
#include "Window.h"
#include "Mouse.h"

using namespace NCL;
using namespace NCL::Maths;
using namespace NCL::CSC8503;

const float PI_OVER_360 = 3.14159265359f / 360.0f;

bool CollisionDetectionDOD::ObjectIntersection(const GameObjectDOD& objA, const GameObjectDOD& objB,
	size_t idxA, size_t idxB, CollisionInfoDOD& collisionInfo) {

	collisionInfo.entityA = idxA;
	collisionInfo.entityB = idxB;

	return AABBIntersection(objA.transform, objA.collision,
		objB.transform, objB.collision, collisionInfo);
}

void CollisionDetectionDOD::DetectAllCollisions(GameObjectStorage& storage, std::vector<CollisionInfoDOD>& outCollisions) {
	outCollisions.clear();

	std::vector<GameObjectDOD>& objects = storage.GetObjectArray();

	for (size_t i = 0; i < objects.size(); ++i) {
		if (!objects[i].isActive) continue;

		for (size_t j = i + 1; j < objects.size(); ++j) {
			if (!objects[j].isActive) continue;

			CollisionInfoDOD collision(i, j);
			if (ObjectIntersection(objects[i], objects[j], i, j, collision)) {
				outCollisions.emplace_back(collision);
			}
		}
	}
}

bool CollisionDetectionDOD::RayPlaneIntersection(const Ray& r, const Plane& p, RayCollisionDOD& collision) {
	float ln = Vector::Dot(p.GetNormal(), r.GetDirection());

	if (ln == 0.0f) {
		collision.hasCollided = false;
		return false;
	}

	Vector3 planePoint = p.GetPointOnPlane();
	Vector3 pointDir = planePoint - r.GetPosition();
	float d = Vector::Dot(pointDir, p.GetNormal()) / ln;

	collision.collidedAt = r.GetPosition() + (r.GetDirection() * d);
	collision.rayDistance = d;
	collision.hasCollided = true;
	return true;
}

bool CollisionDetectionDOD::RayBoxIntersection(const Ray& r, const Vector3& boxPos, const Vector3& boxSize, RayCollisionDOD& collision) {
	Vector3 boxMin = boxPos - boxSize;
	Vector3 boxMax = boxPos + boxSize;

	Vector3 rayPos = r.GetPosition();
	Vector3 rayDir = r.GetDirection();

	Vector3 tVals(-1, -1, -1);

	for (int i = 0; i < 3; ++i) {
		if (rayDir[i] > 0) {
			tVals[i] = (boxMin[i] - rayPos[i]) / rayDir[i];
		}
		else if (rayDir[i] < 0) {
			tVals[i] = (boxMax[i] - rayPos[i]) / rayDir[i];
		}
	}

	float bestT = Vector::GetMaxElement(tVals);

	if (bestT < 0.0f) {
		collision.hasCollided = false;
		return false;
	}

	Vector3 intersection = rayPos + (rayDir * bestT);
	const float epsilon = 0.0001f;
	for (int i = 0; i < 3; ++i) {
		if (intersection[i] + epsilon < boxMin[i] ||
			intersection[i] - epsilon > boxMax[i]) {
			collision.hasCollided = false;
			return false;
		}
	}

	collision.collidedAt = intersection;
	collision.rayDistance = bestT;
	collision.hasCollided = true;
	return true;
}

bool CollisionDetectionDOD::RayAABBIntersection(const Ray& r, const TransformsComp& transform, const AABBComp& volume, RayCollisionDOD& collision) {
	Vector3 boxPos = transform.position;
	Vector3 boxSize = volume.halfSizes;

	return RayBoxIntersection(r, boxPos, boxSize, collision);
}

bool CollisionDetectionDOD::AABBTest(const Vector3& posA, const Vector3& posB, const Vector3& halfSizeA, const Vector3& halfSizeB) {
	Vector3 delta = posB - posA;
	Vector3 totalSize = halfSizeA + halfSizeB;

	if (abs(delta.x) < totalSize.x &&
		abs(delta.y) < totalSize.y &&
		abs(delta.z) < totalSize.z) {
		return true;
	}
	return false;
}

bool CollisionDetectionDOD::AABBIntersection(const TransformsComp& transformA, const AABBComp& volumeA,
	const TransformsComp& transformB, const AABBComp& volumeB, CollisionInfoDOD& collisionInfo) {

	Vector3 boxAPos = transformA.position;
	Vector3 boxBPos = transformB.position;

	Vector3 boxASize = volumeA.halfSizes;
	Vector3 boxBSize = volumeB.halfSizes;

	bool overlap = AABBTest(boxAPos, boxBPos, boxASize, boxBSize);

	if (overlap) {
		static const Vector3 faces[6] = {
			Vector3(-1,  0,  0), Vector3(1, 0,  0),
			Vector3(0, -1,  0), Vector3(0, 1,  0),
			Vector3(0,  0, -1), Vector3(0, 0, 1),
		};

		Vector3 maxA = boxAPos + boxASize;
		Vector3 minA = boxAPos - boxASize;

		Vector3 maxB = boxBPos + boxBSize;
		Vector3 minB = boxBPos - boxBSize;

		float distances[6] = {
			(maxB.x - minA.x),
			(maxA.x - minB.x),
			(maxB.y - minA.y),
			(maxA.y - minB.y),
			(maxB.z - minA.z),
			(maxA.z - minB.z),
		};

		float penetration = FLT_MAX;
		Vector3 bestAxis;

		for (int i = 0; i < 6; ++i) {
			if (distances[i] < penetration) {
				penetration = distances[i];
				bestAxis = faces[i];
			}
		}

		collisionInfo.AddContactPoint(Vector3(), Vector3(), bestAxis, penetration);
		collisionInfo.isActive = true;

		return true;
	}
	return false;
}

Matrix4 CollisionDetectionDOD::GenerateInverseView(const Camera& c) {
	float pitch = c.GetPitch();
	float yaw = c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix::Translation(position) *
		Matrix::Rotation(-yaw, Vector3(0, -1, 0)) *
		Matrix::Rotation(-pitch, Vector3(-1, 0, 0));

	return iview;
}

Matrix4 CollisionDetectionDOD::GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane) {
	Matrix4 m;

	float t = tan(fov * PI_OVER_360);
	float neg_depth = nearPlane - farPlane;
	const float h = 1.0f / t;

	m.array[0][0] = aspect / h;
	m.array[1][1] = 1.0f / t;
	m.array[2][2] = 0.0f;
	m.array[2][3] = -1.0f;
	m.array[3][2] = -2.0f * nearPlane * farPlane / neg_depth;
	m.array[3][3] = (nearPlane + farPlane) / neg_depth;

	return m;
}

Vector3 CollisionDetectionDOD::Unproject(const Vector3& screenPos, const PerspectiveCamera& cam) {
	Vector2i screenSize = Window::GetWindow()->GetScreenSize();

	float aspect = (float)screenSize.x / (float)screenSize.y;
	float fov = cam.GetFieldOfVision();
	float nearPlane = cam.GetNearPlane();
	float farPlane = cam.GetFarPlane();

	Matrix4 invVP = GenerateInverseView(cam) * GenerateInverseProjection(aspect, fov, nearPlane, farPlane);

	Vector4 clipSpace = Vector4(
		(screenPos.x / (float)screenSize.x) * 2.0f - 1.0f,
		(screenPos.y / (float)screenSize.y) * 2.0f - 1.0f,
		(screenPos.z),
		1.0f
	);

	Vector4 transformed = invVP * clipSpace;

	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

Ray CollisionDetectionDOD::BuildRayFromMouse(const PerspectiveCamera& cam) {
	Vector2 screenMouse = Window::GetMouse()->GetAbsolutePosition();
	Vector2i screenSize = Window::GetWindow()->GetScreenSize();

	Vector3 nearPos = Vector3(screenMouse.x, screenSize.y - screenMouse.y, -0.99999f);
	Vector3 farPos = Vector3(screenMouse.x, screenSize.y - screenMouse.y, 0.99999f);

	Vector3 a = Unproject(nearPos, cam);
	Vector3 b = Unproject(farPos, cam);
	Vector3 c = b - a;

	c = Vector::Normalise(c);

	return Ray(cam.GetPosition(), c);
}