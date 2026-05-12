#pragma once
#include "GameWorld.h"
#include "CollisionDetection.h"

struct PhysicsSystemDOD {
	Vector3 gravity;
	float dTOffset;
	float globalDamping;
	int numCollisionFrames;
	bool applyGravity;
	bool useBroadPhase;
};