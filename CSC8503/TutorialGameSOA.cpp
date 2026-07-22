#include "TutorialGameSOA.h"
#include "GameTechRendererInterface.h"
#include "Camera.h"
#include "KeyboardMouseController.h"
#include "Window.h"
#include "TextureLoader.h"
#include "Debug.h"
#include "TransformSOA.h"
#include "PhysicsSystemSOA.h"
#include "GameObjectSOA.h"
#include <chrono>

using namespace NCL;
using namespace CSC8503;

TutorialGameSOA::TutorialGameSOA(GameWorldSOA& inGameWorld, RendererSystemSOA& inRenderer, PhysicsSystemSOA& inPhysics)
	: gameWorld(inGameWorld), rendererSOA(inRenderer), physics(inPhysics), performanceLogger("performance_SOA.csv") {
	data.useGravity = true;

	performanceLogger.SetLogInterval(1.0);

	physics.UseGravity(data.useGravity);

	controller = new KeyboardMouseController(*Window::GetWindow()->GetKeyboard(), *Window::GetWindow()->GetMouse());
	gameWorld.GetMainCamera().SetController(*controller);

	gameWorld.SetSunPosition(Vector3(-200.0f, 60.0f, -200.0f));
	gameWorld.SetSunColour(Vector3(0.8f, 0.8f, 0.5f));

	controller->MapAxis(0, "Sidestep");
	controller->MapAxis(1, "UpDown");
	controller->MapAxis(2, "Forward");

	controller->MapAxis(3, "XLook");
	controller->MapAxis(4, "YLook");

	data.x = 150;
	data.y = 150;

	InitCamera();
	LoadResources();
	InitWorld();
}

TutorialGameSOA::~TutorialGameSOA() {
	performanceLogger.Flush();
	if (controller) {
		delete controller;
	}
}

void TutorialGameSOA::InitCamera() {
	gameWorld.GetMainCamera().SetNearPlane(0.1f);
	gameWorld.GetMainCamera().SetFarPlane(500.0f);
	gameWorld.GetMainCamera().SetPitch(-15.0f);
	gameWorld.GetMainCamera().SetYaw(315.0f);
	gameWorld.GetMainCamera().SetPosition(Vector3(-60, 40, 60));
}

void TutorialGameSOA::LoadResources() {
	resources.cubeMesh = rendererSOA.LoadMesh("cube.msh");
	resources.sphereMesh = rendererSOA.LoadMesh("sphere.msh");

	resources.checkerTex = rendererSOA.LoadTexture("checkerboard.png");

	resources.checkerMaterial.type = MaterialType::Opaque;
	resources.checkerMaterial.diffuseTex = resources.checkerTex;

	std::cout << "Cube Mesh: " << (resources.cubeMesh ? "LOADED" : "NULL") << std::endl;
	std::cout << "Checker Texture: " << (resources.checkerTex ? "LOADED" : "NULL") << std::endl;
	std::cout << "Material Type: " << (int)resources.checkerMaterial.type << std::endl;
}

void TutorialGameSOA::InitWorld() {
	gameWorld.Clear();
	physics.Clear();
	InitTest();
}

void TutorialGameSOA::UpdateGame(float dt) {
	timingDisplay.RecordFrameTime(dt);

	gameWorld.GetMainCamera().UpdateCamera(dt);

	// Physics timing
	timingDisplay.StartPhysicsTiming();
	physics.Update(dt);
	timingDisplay.EndPhysicsTiming();

	// Rendering timing
	timingDisplay.StartRenderTiming();
	auto& gameObjects = gameWorld.gameObjects;
	int objectCount = GameObjectOpsSOA::GetObjectCount(gameObjects);
	//volatile float accumulator = 0.0f;

	////// Update render objects
	//for (int i = 0; i < objectCount; ++i) {
	//	if (gameObjects.isActive[i]) {
	//		const Vector3& pos = gameObjects.transforms.positions[i];
	//		float mass = gameObjects.physics.inverseMassSOA[i];
	//		const Vector3& aabb = gameObjects.collision.AABBDataSOA.halfSizesSOA[i];

	//		accumulator += pos.x + mass + aabb.x;
	//	}
	//}

	timingDisplay.EndRenderTiming();

	// SoA doesn't have garbage data in the hot path, so wasted memory is 0
	//size_t wastedMemory = 0;

	// Log performance data
	performanceLogger.Update(dt, timingDisplay, objectCount);

	// Display timing information
	const auto& timingData = timingDisplay.GetTimingData();

	Debug::Print("Current FPS: " + std::to_string((int)(1.0f / dt)), Vector2(0, 5), Debug::WHITE);
	Debug::Print("Avg FPS: " + TimingDisplay::FormatFPS(timingData.averageFPS), Vector2(0, 10), Debug::WHITE);
	Debug::Print("Avg Frame Time: " + TimingDisplay::FormatTime(timingData.averageFrameTimeMs) + " ms", Vector2(0, 15), Debug::WHITE);
	Debug::Print("Avg Physics Time: " + TimingDisplay::FormatTime(timingData.averagePhysicsTimeMs) + " ms", Vector2(0, 20), Debug::WHITE);
	Debug::Print("Avg Render Time: " + TimingDisplay::FormatTime(timingData.averageRenderTimeMs) + " ms", Vector2(0, 25), Debug::WHITE);
	Debug::Print("Objects: " + std::to_string(objectCount), Vector2(0, 30), Debug::WHITE);
	//Debug::Print("Wasted Memory: 0 MB", Vector2(0, 35), Debug::WHITE);
	//Debug::Print("Logged Records: " + std::to_string(performanceLogger.GetRecordCount()), Vector2(0, 40), Debug::WHITE);
}

void TutorialGameSOA::InitTest() {
	gameWorld.Clear();
	physics.Clear();
	physics.data.useBroadPhase = true;

	int estimatedCapacity = 1 + (data.x * data.y);
	gameWorld.reserveCapacity(estimatedCapacity);

	AddFloorToWorld(Vector3(0, -5, 0), 2, 10000);
	std::cout << "Floor added. Total objects: " << gameWorld.GetObjectCount() << std::endl;

	CreateAABBGrid(data.x, data.y, 5.0f, 5.0f, Vector3(1, 1, 1));
	std::cout << "Cubes added. Total objects: " << gameWorld.GetObjectCount() << std::endl;
}

int TutorialGameSOA::AddFloorToWorld(const Vector3& position, float floorHeight, float floorLength, int collisionLayer) {
	int index = GameObjectOpsSOA::AddGameObject(gameWorld.gameObjects, GameObjectType::Default);

	TransformOpsSOA::SetPositionSOA(gameWorld.gameObjects.transforms, position, index);
	TransformOpsSOA::SetScaleSOA(gameWorld.gameObjects.transforms, Vector3(floorLength, floorHeight, floorLength), index);

	gameWorld.gameObjects.collision.AABBDataSOA.halfSizesSOA[index] = Vector3(floorLength * 0.5f, floorHeight * 0.5f, floorLength * 0.5f);

	PhysicsOpsSOA::SetInverseMass(gameWorld.gameObjects.physics, 0.0f, index);

	RenderOpsSOA::SetMesh(gameWorld.gameObjects.render, index, resources.cubeMesh);
	RenderOpsSOA::SetDiffuseTexture(gameWorld.gameObjects.render, index, resources.checkerTex);
	RenderOpsSOA::SetMaterialType(gameWorld.gameObjects.render, index, MaterialType::Opaque);
	gameWorld.gameObjects.render.colours[index] = Vector4(0.9f, 0.9f, 0.9f, 1.0f);

	gameWorld.gameObjects.isActive[index] = true;
	gameWorld.gameObjects.collisionLayers[index] = collisionLayer;

	return index;
}

int TutorialGameSOA::AddCubeToWorld(const Vector3& position, const Vector3& cubeDims, float inverseMass, int collisionLayer) {
	int index = GameObjectOpsSOA::AddGameObject(gameWorld.gameObjects, GameObjectType::Default);

	TransformOpsSOA::SetPositionSOA(gameWorld.gameObjects.transforms, position, index);
	TransformOpsSOA::SetScaleSOA(gameWorld.gameObjects.transforms, cubeDims, index);

	gameWorld.gameObjects.collision.AABBDataSOA.halfSizesSOA[index] = cubeDims * 0.5f;

	PhysicsOpsSOA::SetInverseMass(gameWorld.gameObjects.physics, inverseMass, index);
	PhysicsOpsSOA::InitCubeInertia(gameWorld.gameObjects.physics, cubeDims, index);
	PhysicsOpsSOA::UpdateInertiaTensor(gameWorld.gameObjects.physics, gameWorld.gameObjects.transforms.orientations[index], index);

	RenderOpsSOA::SetMesh(gameWorld.gameObjects.render, index, resources.cubeMesh);
	RenderOpsSOA::SetDiffuseTexture(gameWorld.gameObjects.render, index, resources.checkerTex);
	RenderOpsSOA::SetMaterialType(gameWorld.gameObjects.render, index, MaterialType::Opaque);
	gameWorld.gameObjects.render.colours[index] = Vector4(
		0.5f + (rand() / (float)RAND_MAX) * 0.5f,
		0.5f + (rand() / (float)RAND_MAX) * 0.5f,
		0.5f + (rand() / (float)RAND_MAX) * 0.5f,
		1.0f
	);

	gameWorld.gameObjects.isActive[index] = true;
	gameWorld.gameObjects.collisionLayers[index] = collisionLayer;

	return index;
}

void TutorialGameSOA::CreateAABBGrid(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols + 1; ++x) {
		for (int z = 1; z < numRows + 1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims * Vector3(2, 2, 2), 1.0f);
		}
	}
}