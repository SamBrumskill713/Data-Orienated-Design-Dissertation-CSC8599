#include "TutorialGame.h"
#include "GameWorld.h"
#include "PhysicsSystem.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "TextureLoader.h"

#include "Window.h"
#include "Texture.h"
#include "Shader.h"
#include "Mesh.h"

#include "Debug.h"

#include "KeyboardMouseController.h"

#include "GameTechRendererInterface.h"

#include "Ray.h"

using namespace NCL;
using namespace CSC8503;

TutorialGame::TutorialGame(GameWorld& inWorld, GameTechRendererInterface& inRenderer, PhysicsSystem& inPhysics)
	: world(inWorld),
	renderer(inRenderer),
	physics(inPhysics)
{
	forceMagnitude = 10.0f;
	useGravity = true;

	controller = new KeyboardMouseController(*Window::GetWindow()->GetKeyboard(), *Window::GetWindow()->GetMouse());

	world.GetMainCamera().SetController(*controller);

	world.SetSunPosition({ -200.0f, 60.0f, -200.0f });
	world.SetSunColour({ 0.8f, 0.8f, 0.5f });

	controller->MapAxis(0, "Sidestep");
	controller->MapAxis(1, "UpDown");
	controller->MapAxis(2, "Forward");

	controller->MapAxis(3, "XLook");
	controller->MapAxis(4, "YLook");

	cubeMesh = renderer.LoadMesh("cube.msh");

	//defaultTex = renderer.LoadTexture("Default.png");
	checkerTex = renderer.LoadTexture("checkerboard.png");

	checkerMaterial.type = MaterialType::Opaque;
	checkerMaterial.diffuseTex = checkerTex;

	InitCamera();
	InitWorld();
}

TutorialGame::~TutorialGame() {
	if (controller) {
		delete controller;
		controller = nullptr;
	}
}

void TutorialGame::UpdateGame(float dt) {

	world.GetMainCamera().UpdateCamera(dt);
	if (useGravity) physics.UseGravity(useGravity);

	frameTimeSamples.emplace_back(dt);
	if (frameTimeSamples.size() > FPS_SAMPLE_SIZE) {
		frameTimeSamples.erase(frameTimeSamples.begin());
	}

	float totalTime = 0.0f;
	for (float sample : frameTimeSamples) {
		totalTime += sample;
	}
	averageFPS = frameTimeSamples.size() / totalTime;

	Debug::Print("Current FPS: " + std::to_string((int)(1.0f / dt)), Vector2(0, 5), Debug::WHITE);
	Debug::Print("Avg FPS: " + std::to_string((int)averageFPS), Vector2(0, 10), Debug::WHITE);
	GameObjectIterator first, last;
	world.GetObjectIterators(first, last);
	int objectCount = std::distance(first, last);
	Debug::Print("Objects: " + std::to_string(objectCount), Vector2(0, 15), Debug::WHITE);

	physics.Update(dt);
	world.OperateOnContents([dt](GameObject* o) { o->Update(dt); });
}

void TutorialGame::InitCamera() {
	world.GetMainCamera().SetNearPlane(0.1f);
	world.GetMainCamera().SetFarPlane(500.0f);
	world.GetMainCamera().SetPitch(-15.0f);
	world.GetMainCamera().SetYaw(315.0f);
	world.GetMainCamera().SetPosition(Vector3(-60, 40, 60));
}



void TutorialGame::InitWorld() {
	world.ClearAndErase();
	physics.Clear();
	InitFPSTest();
}

GameObject* TutorialGame::AddFloorToWorld(const Vector3& position, float floorHeight, float floorLength,
	bool isTrigger, int collisionLayer) {
	GameObject* floor = new GameObject();

	Vector3 floorSize = Vector3(floorHeight, 2, floorLength);
	AABBVolume* volume = new AABBVolume(floorSize, isTrigger);
	volume->collisionLayer = collisionLayer;
	floor->SetBoundingVolume(volume);
	floor->GetTransform()
		.SetScale(floorSize * 2.0f)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(floor->GetTransform(), cubeMesh, checkerMaterial));
	floor->SetPhysicsObject(new PhysicsObject(floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world.AddGameObject(floor);

	return floor;
}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass,
	bool isTrigger, int collisionLayer) {
	GameObject* cube = new GameObject();

	AABBVolume* volume = new AABBVolume(dimensions, isTrigger);
	volume->collisionLayer = collisionLayer;
	cube->SetBoundingVolume(volume);
	cube->setIsCollided(!isTrigger);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2.0f);

	cube->SetRenderObject(new RenderObject(cube->GetTransform(), cubeMesh, checkerMaterial));
	cube->SetPhysicsObject(new PhysicsObject(cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();
	cube->GetRenderObject()->SetColour(Vector4(
		0.5f + (rand() / (float)RAND_MAX) * 0.5f,
		0.5f + (rand() / (float)RAND_MAX) * 0.5f,
		0.5f + (rand() / (float)RAND_MAX) * 0.5f,
		1.0f));

	world.AddGameObject(cube);

	return cube;
}

void NCL::CSC8503::TutorialGame::InitFPSTest()
{
	world.Clear();
	physics.Clear();
	AddFloorToWorld(Vector3(0, -5, 0), 10000, 10000);
	CreateAABBGrid(50, 50, 5.0f, 5.0f, Vector3(1, 1, 1));
	
}

void TutorialGame::CreateAABBGrid(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols + 1; ++x) {
		for (int z = 1; z < numRows + 1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
}