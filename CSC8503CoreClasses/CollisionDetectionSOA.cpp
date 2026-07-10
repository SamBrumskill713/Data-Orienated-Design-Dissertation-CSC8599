#include "CollisionDetectionSOA.h"
#include "Window.h"

using namespace NCL::CSC8503;
using namespace NCL::Maths;


CollisionInfoSOA::CollisionInfoSOA()
	: entityA(-1), entityB(-1), framesLeft(0), normal(Vector3()),
	penetration(0.0f), isActive(false) {
}

CollisionInfoSOA::CollisionInfoSOA(int a, int b)
	: entityA(a), entityB(b), framesLeft(0), normal(Vector3()),
	penetration(0.0f), isActive(false) {
}

void CollisionInfoSOA::AddContactPoint(const Vector3& lA, const Vector3& lB, const Vector3& norm, float pene) {
	localA = lA;
	localB = lB;
	normal = norm;
	penetration = pene;
}


RayCollisionSOA::RayCollisionSOA()
	: node(nullptr), collidedAt(Vector3()),
	rayDistance(FLT_MAX), hasCollided(false) {
}


bool CollisionDetectionSOA::ObjectIntersection(const GameObjectCompSOA& gameObjects,
	int idxA, int idxB, CollisionInfoSOA& collisionInfo) {
	collisionInfo.entityA = idxA;
	collisionInfo.entityB = idxB;

	return AABBIntersection(idxA, idxB, gameObjects, collisionInfo);
}

void CollisionDetectionSOA::DetectAllCollisions(GameObjectCompSOA& gameObjects,
	std::vector<CollisionInfoSOA>& outCollisions) {
	outCollisions.clear();

	std::vector<int> activeObjects;
	GameObjectOpsSOA::GetActiveObjects(gameObjects, activeObjects);

	for (size_t i = 0; i < activeObjects.size(); ++i) {
		for (size_t j = i + 1; j < activeObjects.size(); ++j) {
			int idxA = activeObjects[i];
			int idxB = activeObjects[j];

			int layerA = gameObjects.collisionLayers[idxA];
			int layerB = gameObjects.collisionLayers[idxB];

			if (!CollisionMatrix[layerA][layerB]) {
				continue;
			}

			CollisionInfoSOA collisionInfo(idxA, idxB);
			if (ObjectIntersection(gameObjects, idxA, idxB, collisionInfo)) {
				collisionInfo.isActive = true;
				outCollisions.emplace_back(collisionInfo);

				gameObjects.isCollided[idxA] = true;
				gameObjects.isCollided[idxB] = true;
			}
		}
	}
}

bool CollisionDetectionSOA::AABBTest(const Vector3& posA, const Vector3& posB,
	const Vector3& halfSizeA, const Vector3& halfSizeB) {
	Vector3 delta = posB - posA;

	float overlapX = (halfSizeA.x + halfSizeB.x) - abs(delta.x);
	float overlapY = (halfSizeA.y + halfSizeB.y) - abs(delta.y);
	float overlapZ = (halfSizeA.z + halfSizeB.z) - abs(delta.z);

	return overlapX > 0.0f && overlapY > 0.0f && overlapZ > 0.0f;
}

bool CollisionDetectionSOA::AABBIntersection(int indexA, int indexB,
	const GameObjectCompSOA& gameObjects,
	CollisionInfoSOA& collisionInfo) {
	const Vector3& posA = gameObjects.transforms.positions[indexA];
	const Vector3& posB = gameObjects.transforms.positions[indexB];

	Vector3 halfSizeA = gameObjects.collision.AABBDataSOA.halfSizesSOA[indexA];
	Vector3 halfSizeB = gameObjects.collision.AABBDataSOA.halfSizesSOA[indexB];

	Vector3 delta = posB - posA;

	float overlapX = (halfSizeA.x + halfSizeB.x) - abs(delta.x);
	float overlapY = (halfSizeA.y + halfSizeB.y) - abs(delta.y);
	float overlapZ = (halfSizeA.z + halfSizeB.z) - abs(delta.z);

	if (overlapX > 0.0f && overlapY > 0.0f && overlapZ > 0.0f) {
		float penetration = std::min({ overlapX, overlapY, overlapZ });

		Vector3 normal(0, 0, 0);

		if (penetration == overlapX) {
			normal.x = delta.x > 0.0f ? 1.0f : -1.0f;
		}
		else if (penetration == overlapY) {
			normal.y = delta.y > 0.0f ? 1.0f : -1.0f;
		}
		else {
			normal.z = delta.z > 0.0f ? 1.0f : -1.0f;
		}
		collisionInfo.AddContactPoint(Vector3(0, 0, 0), Vector3(0, 0, 0), normal, penetration);
		return true;
	}

	return false;
}

bool CollisionDetectionSOA::RayPlaneIntersection(const Ray& r, const Plane& p, RayCollisionSOA& collision) {
	Vector3 planeNormal = p.GetNormal();
	Vector3 rayDir = r.GetDirection();
	float denominator = Vector::Dot(planeNormal, rayDir);

	if (abs(denominator) > 0.0001f) {
		float t = Vector::Dot(planeNormal, p.GetPointOnPlane() - r.GetPosition()) / denominator;
		if (t >= 0.0f) {
			collision.collidedAt = r.GetPosition() + rayDir * t;
			collision.rayDistance = t;
			collision.hasCollided = true;
			return true;
		}
	}

	return false;
}

bool CollisionDetectionSOA::RayBoxIntersection(const Ray& r, const Vector3& boxPos,
	const Vector3& boxSize, RayCollisionSOA& collision) {
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

bool CollisionDetectionSOA::RayAABBIntersection(const Ray& r, int objectIndex,
	const GameObjectCompSOA& gameObjects,
	RayCollisionSOA& collision) {
	const Vector3& pos = gameObjects.transforms.positions[objectIndex];
	Vector3 halfSize(0.5f, 0.5f, 0.5f);

	return RayBoxIntersection(r, pos, halfSize * 2.0f, collision);
}

Vector3 CollisionDetectionSOA::Unproject(const Vector3& screenPos, const PerspectiveCamera& cam) {
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

Ray CollisionDetectionSOA::BuildRayFromMouse(const PerspectiveCamera& cam) {
	Vector3 nearPoint = Unproject(Vector3(0, 0, 0), cam);
	Vector3 farPoint = Unproject(Vector3(0, 0, 1), cam);
	Vector3 direction = farPoint - nearPoint;
	direction = Vector::Normalise(direction);

	return Ray(nearPoint, direction);
}

Matrix4 CollisionDetectionSOA::GenerateInverseView(const Camera& c) {
	float pitch = c.GetPitch();
	float yaw = c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix::Translation(position) *
		Matrix::Rotation(-yaw, Vector3(0, -1, 0)) *
		Matrix::Rotation(-pitch, Vector3(-1, 0, 0));

	return iview;
}

Matrix4 CollisionDetectionSOA::GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane) {
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