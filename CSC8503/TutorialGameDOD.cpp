#include "TutorialGameDOD.h"
#include "GameTechRendererInterface.h"
#include "Camera.h"
#include "KeyboardMouseController.h"
#include "Window.h"
#include "TextureLoader.h"
#include "Debug.h"
#include "TransformDOD.h"
#include "PhysicsSystemDOD.h"

using namespace NCL;
using namespace CSC8503;

TutorialGameDOD::TutorialGameDOD(GameWorldDOD& inGameWold, RendererSystemDOD& inRenderer, PhysicsSystemDOD& inPhysics)
	: gameWorld(inGameWold), rendererDOD(inRenderer), physics(inPhysics) {
	data.useGravity = true;

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

	InitCamera();
	LoadResources();
	InitWorld();
}

TutorialGameDOD::~TutorialGameDOD() {
}

void TutorialGameDOD::InitCamera() {
	gameWorld.GetMainCamera().SetNearPlane(0.1f);
	gameWorld.GetMainCamera().SetFarPlane(500.0f);
	gameWorld.GetMainCamera().SetPitch(-15.0f);
	gameWorld.GetMainCamera().SetYaw(315.0f);
	gameWorld.GetMainCamera().SetPosition(Vector3(-60, 40, 60));
}

void TutorialGameDOD::LoadResources() {
	resources.cubeMesh = rendererDOD.LoadMesh("cube.msh");
	resources.sphereMesh = rendererDOD.LoadMesh("sphere.msh");

	resources.checkerTex = rendererDOD.LoadTexture("checkerboard.png");

	resources.checkerMaterial.type = MaterialType::Opaque;
	resources.checkerMaterial.diffuseTex = resources.checkerTex;

	// Debug output
	std::cout << "Cube Mesh: " << (resources.cubeMesh ? "LOADED" : "NULL") << std::endl;
	std::cout << "Checker Texture: " << (resources.checkerTex ? "LOADED" : "NULL") << std::endl;
	std::cout << "Material Type: " << (int)resources.checkerMaterial.type << std::endl;
}

void TutorialGameDOD::InitWorld() {
	gameWorld.Clear();
	physics.Clear();
	InitTest();
}

void TutorialGameDOD::UpdateGame(float dt) {
	gameWorld.GetMainCamera().UpdateCamera(dt);

	physics.Update(dt);

	gameWorld.OperateOnContents([this](GameObjectDOD& obj) {
		/*if (obj.transform.position.y < -100.0f) {
			obj.isActive = false;
		}*/
	});

	Debug::Print("FPS: " + std::to_string((int)(1.0f / dt)), Vector2(0, 5), Debug::WHITE);
	Debug::Print("Objects: " + std::to_string(gameWorld.GetObjectCount()), Vector2(0, 10), Debug::WHITE);
}

void TutorialGameDOD::InitTest() {
	gameWorld.Clear();
	physics.Clear();
	physics.data.useBroadPhase = true;
	AddFloorToWorld(Vector3(0, -5, 0), 2, 10000);
	std::cout << "Floor added. Total objects: " << gameWorld.GetObjectCount() << std::endl;
	CreateAABBGrid(20, 20, 5.0f, 5.0f, Vector3(1, 1, 1));
	std::cout << "Cubes added. Total objects: " << gameWorld.GetObjectCount() << std::endl;
}

size_t NCL::CSC8503::TutorialGameDOD::AddFloorToWorld(const Vector3& position, float floorHeight, float floorLength, int collisionLayer)
{
	GameObjectDOD& floorObj = gameWorld.gameObjects.AddObject("Floor");

	TransformOps::SetPosition(floorObj.transform, position);
	TransformOps::SetScale(floorObj.transform, Vector3(floorLength, floorHeight, floorLength));

	floorObj.collision.halfSizes = Vector3(floorLength * 0.5f, floorHeight * 0.5f, floorLength * 0.5f);  // HALF SIZES!
	floorObj.physics.inverseMass = 0.0f;
	floorObj.render.mesh = resources.cubeMesh;
	floorObj.render.material = resources.checkerMaterial;
	floorObj.render.colour = Vector4(0.9f, 0.9f, 0.9f, 1.0f);
	floorObj.isActive = true;

	gameWorld.AddGameObject(floorObj);
	return gameWorld.gameObjects.GetObjectCount() - 1;
}

size_t NCL::CSC8503::TutorialGameDOD::addCubeToWorld(const Vector3& position, const Vector3& cubeDims, float inverseMass, int collisionLayer)
{
	GameObjectDOD& cubeObj = gameWorld.gameObjects.AddObject("Cube");

	TransformOps::SetPosition(cubeObj.transform, position);
	TransformOps::SetScale(cubeObj.transform, cubeDims);

	cubeObj.collision.halfSizes = cubeDims * 0.5f;  // HALF SIZES!
	cubeObj.physics.inverseMass = inverseMass;
	PhysicsOps::InitCubeInertia(cubeObj.physics, cubeDims);
	PhysicsOps::UpdateInertiaTensor(cubeObj.physics, cubeObj.transform.orientation);

	cubeObj.render.mesh = resources.cubeMesh;
	cubeObj.render.material = resources.checkerMaterial;
	cubeObj.render.colour = Vector4(
		0.5f + (rand() / (float)RAND_MAX) * 0.5f,
		0.5f + (rand() / (float)RAND_MAX) * 0.5f,
		0.5f + (rand() / (float)RAND_MAX) * 0.5f,
		1.0f
	);
	cubeObj.isActive = true;

	gameWorld.AddGameObject(cubeObj);
	return gameWorld.gameObjects.GetObjectCount() - 1;
}

void TutorialGameDOD::CreateAABBGrid(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols + 1; ++x) {
		for (int z = 1; z < numRows + 1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			addCubeToWorld(position, cubeDims * Vector3(2, 2, 2), 1.0f);
		}
	}
}
