#include "TutorialGame.h"
#include "GameWorld.h"
#include "PhysicsSystem.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "TextureLoader.h"
#include "../CSC8503CoreClasses/NavigationGrid.h"

#include "PositionConstraint.h"
#include "OrientationConstraint.h"
#include "StateGameObject.h"

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
	inSelectionMode = false;

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
	sphereMesh = renderer.LoadMesh("sphere.msh");
	catMesh = renderer.LoadMesh("ORIGAMI_Chat.msh");
	kittenMesh = renderer.LoadMesh("Kitten.msh");

	enemyMesh = renderer.LoadMesh("Goat.msh");
	playerMesh = renderer.LoadMesh("Keeper.msh");

	//bonusMesh = renderer.LoadMesh("19463_Kitten_Head_v1.msh");
	capsuleMesh = renderer.LoadMesh("capsule.msh");

	//defaultTex = renderer.LoadTexture("Default.png");
	checkerTex = renderer.LoadTexture("checkerboard.png");
	glassTex = renderer.LoadTexture("stainedglass.tga");

	checkerMaterial.type = MaterialType::Opaque;
	checkerMaterial.diffuseTex = checkerTex;

	glassMaterial.type = MaterialType::Transparent;
	glassMaterial.diffuseTex = glassTex;

	InitCamera();
	InitWorld();
}

TutorialGame::~TutorialGame() {
}

void TutorialGame::UpdateGame(float dt) {
	/*if (isGameOver || isWin) {
		if (isWin) {
			Debug::Print("YOU WIN!",             Vector2(35, 45), Debug::GREEN);
			Debug::Print("All items delivered.", Vector2(30, 50), Debug::WHITE);
		} else {
			Debug::Print("GAME OVER!",        Vector2(33, 45), Debug::RED);
			Debug::Print("Time ran out.",     Vector2(34, 50), Debug::WHITE);
		}
		Debug::Print("Press ESC to quit", Vector2(32, 60), Debug::WHITE);
		return;
	}*/

	world.GetMainCamera().UpdateCamera(dt);
	if (useGravity) physics.UseGravity(useGravity);

	/*gameTime -= dt;*/


	//if (allowLocalPlayerControl && playerObj && playerGroundCollision && levelFloor) {
	//	playerGroundCollision->GetTransform().SetPosition(
	//		playerObj->GetTransform().GetPosition() + Vector3(0, -3.0f, 0));
	//	movePlayerObject(dt);

	//	attachCameraToPlayer(); 

	//	
	//	/*Debug::debugDrawAABBs(playerObj->GetTransform().GetPosition(),
	//		Vector3(0.3f, 0.9f, 0.3f) * 3.0f, Debug::RED, 0.0f);
	//	Debug::debugDrawAABBs(levelFloor->GetTransform().GetPosition(), Vector3(50, 2, 50), Debug::GREEN, 0.01f);
	//	Debug::debugDrawSphere(playerGroundCollision->GetTransform().GetPosition(), 0.5f, Debug::BLUE, 0.0f, 16);
	//	Debug::Print("Player X:" + std::to_string(playerObj->GetTransform().GetPosition().x), Vector2(0, 10), Debug::WHITE);
	//	Debug::Print("Player Y:" + std::to_string(playerObj->GetTransform().GetPosition().y), Vector2(0, 15), Debug::WHITE);
	//	Debug::Print("Player Z:" + std::to_string(playerObj->GetTransform().GetPosition().z), Vector2(0, 20), Debug::WHITE);
	//	Debug::Print("isCollided: " + std::to_string(playerGroundCollision->getIsCollided()), Vector2(0, 25), Debug::WHITE);
	//	Debug::Print("player isCollided: " + std::to_string(playerObj->getIsCollided()), Vector2(0, 30), Debug::WHITE);
	//	Debug::Print("player isTrigger: " + std::to_string(playerObj->GetBoundingVolume()->isTrigger), Vector2(0, 45), Debug::WHITE);
	//	Debug::Print("player Inventory Size: " + std::to_string(playerObj->getpickUpSize()), Vector2(0, 50), Debug::WHITE);*/
	//	// Score is now printed in the unified HUD below
	//}

	/*if (!allowLocalPlayerControl && cameraTarget) {
		attachCameraToPlayer();
	}*/

	//if (trigVol) {
	//	/*Debug::debugDrawAABBs(trigVol->GetTransform().GetPosition(), trigVol->GetTransform().GetScale(),
	//		Debug::BLUE, 0.1f);*/
	//}

	//if (outOfBounds) {
	//	/*Debug::debugDrawAABBs(outOfBounds->GetTransform().GetPosition(), outOfBounds->GetTransform().GetScale(),
	//		Debug::BLUE, 0.1f);*/
	//}

	/*int itemsRemaining = 0;
	if (data) {
		for (auto* p : levelItems) {
			if (p && p->getIsRendered()) {
				++itemsRemaining;
			}
		}

		levelItems.erase(
			std::remove_if(levelItems.begin(), levelItems.end(),
				[](pickUpObject* p) {
					return p == nullptr || !p->getIsRendered();
				}),
			levelItems.end()
		);

		if (!isWin && levelItems.empty()) {
			std::cout << "win\n";
			Debug::Print("You Got all the items delivered. You Win!", Vector2(0, 85));
			isWin = true; 
		}

		if (!isGameOver && gameTime <= 0.0f) {
			Debug::Print("Game Over!", Vector2(0, 70));
			gameTime = 0.0f;
			isGameOver = true; 
		}
	}

	{
		playerObject* scorePlayer = nullptr;
		if (allowLocalPlayerControl && playerObj) {
			scorePlayer = playerObj;
		} else if (cameraTarget) {
			scorePlayer = dynamic_cast<playerObject*>(cameraTarget);
		}
		const int score = scorePlayer ? scorePlayer->getScore() : 0;

		const float timeLeft = std::max(0.0f, gameTime);
		Debug::Print("Items Remaining: " + std::to_string(itemsRemaining), Vector2(0, 20), Debug::WHITE);
		Debug::Print("Time Left: " + std::to_string((int)timeLeft), Vector2(0, 15), Debug::WHITE);
		Debug::Print("Player Score: " + std::to_string(score), Vector2(0, 10), Debug::WHITE);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F)) {
		world.Clear();
		physics.Clear();
		initAITest();
	}

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::I)) {
		world.Clear();
		physics.Clear();
		InitTriggerTest();
	}

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::B)) {
		world.Clear();
		physics.Clear();
		initObstacleTest();
	}*/

	physics.Update(dt);

	world.OperateOnContents([dt](GameObject* o) { o->Update(dt); });

	Debug::Print("FPS: " + std::to_string((int)(1.0f / dt)), Vector2(0, 5), Debug::WHITE);
	
	//Debug::Print("Objects: " + std::to_string(gameWorld.GetObjectCount()), Vector2(0, 10), Debug::WHITE);

	/*if (testStateObject) testStateObject->Update(dt);*/

	/*if (!inSelectionMode) {
		world.GetMainCamera().UpdateCamera(dt);
	}*/
	//if (lockedObject != nullptr) {
	//	Vector3 objPos = lockedObject->GetTransform().GetPosition();
	//	Vector3 camPos = objPos + lockedOffset;

	//	Matrix4 temp = Matrix::View(camPos, objPos, Vector3(0, 1, 0));

	//	Matrix4 modelMat = Matrix::Inverse(temp);

	//	Quaternion q(modelMat);
	//	Vector3 angles = q.ToEuler(); //nearly there now!

	//	world.GetMainCamera().SetPosition(camPos);
	//	world.GetMainCamera().SetPitch(angles.x);
	//	world.GetMainCamera().SetYaw(angles.y);
	//}

	//if (Window::GetKeyboard()->KeyPressed(KeyCodes::F1)) {
	//	InitWorld(); //We can reset the simulation at any time with F1
	//	selectionObject = nullptr;
	//}

	//if (Window::GetKeyboard()->KeyPressed(KeyCodes::F2)) {
	//	InitCamera(); //F2 will reset the camera to a specific default place
	//}
	// 
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	//if (Window::GetKeyboard()->KeyPressed(KeyCodes::F9)) {
	//	world.ShuffleConstraints(true);
	//}
	//if (Window::GetKeyboard()->KeyPressed(KeyCodes::F10)) {
	//	world.ShuffleConstraints(false);
	//}

	//if (Window::GetKeyboard()->KeyPressed(KeyCodes::F7)) {
	//	world.ShuffleObjects(true);
	//}
	//if (Window::GetKeyboard()->KeyPressed(KeyCodes::F8)) {
	//	world.ShuffleObjects(false);
	//}

	//if (lockedObject) {
	//	LockedObjectMovement();
	//}
	//else {
	//	DebugObjectMovement();
	//}

	/*RayCollision closestCollision;
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::K) && selectionObject) {
		Vector3 rayPos;
		Vector3 rayDir;

		rayDir = selectionObject->GetTransform().GetOrientation() * Vector3(0, 0, -1);

		rayPos = selectionObject->GetTransform().GetPosition();

		Ray r = Ray(rayPos, rayDir);

		if (world.Raycast(r, closestCollision, true, selectionObject)) {
			if (objClosest) {
				objClosest->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
			}
			objClosest = (GameObject*)closestCollision.node;

			objClosest->GetRenderObject()->SetColour(Vector4(1, 0, 1, 1));
		}
	}*/

	//This year we can draw debug textures as well!
	//Debug::DrawTex(*defaultTex, Vector2(10, 10), Vector2(5, 5), Debug::WHITE);
	//Debug::DrawLine(Vector3(), Vector3(0, 100, 0), Vector4(1, 0, 0, 1));
	/*if (useGravity) {
		Debug::Print("(G)ravity on", Vector2(5, 95), Debug::RED);
	}
	else {
		Debug::Print("(G)ravity off", Vector2(5, 95), Debug::RED);
	}*/

	/*SelectObject();
	MoveSelectedObject();*/
}

int TutorialGame::GetPlayerScore() const {

	if (allowLocalPlayerControl && playerObj) {
		return playerObj->getScore();
	}

	if (cameraTarget) {
		if (auto* p = dynamic_cast<playerObject*>(cameraTarget)) {
			return p->getScore();
		}
	}
	return 0;
}

void TutorialGame::InitCamera() {
	world.GetMainCamera().SetNearPlane(0.1f);
	world.GetMainCamera().SetFarPlane(500.0f);
	world.GetMainCamera().SetPitch(-15.0f);
	world.GetMainCamera().SetYaw(315.0f);
	world.GetMainCamera().SetPosition(Vector3(-60, 40, 60));
	lockedObject = nullptr;
}

void NCL::CSC8503::TutorialGame::attachCameraToPlayer() {
    GameObject* target = allowLocalPlayerControl ? playerObj : cameraTarget;
    if (!target || !target->GetRenderObject()) {
        return;
    }

    const float camYaw = world.GetMainCamera().GetYaw();
    const Quaternion newOri = Quaternion::EulerAnglesToQuaternion(0.0f, camYaw, 0.0f);

    if (allowLocalPlayerControl) {
        target->GetTransform().SetOrientation(newOri);
    }

    const Transform& t = target->GetTransform();
    const Vector3 pos = t.GetPosition();
    const Vector3 fwd = newOri * Vector3(0, 0, -1);
    const Vector3 up  = newOri * Vector3(0, 1, 0);

    const float followDistance = 10.0f;
    const float followHeight   = 4.0f;
    const Vector3 camPos = pos - fwd * followDistance + up * followHeight;
    world.GetMainCamera().SetPosition(camPos);
}

void TutorialGame::movePlayerObject(float dt)
{
	if (!playerObj) {
		return;
	}

	if (!allowLocalPlayerControl) {
		return;
	}

	Matrix4 view = world.GetMainCamera().BuildViewMatrix();
	Matrix4 camWorld = Matrix::Inverse(view);

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); 

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis = Vector::Normalise(fwdAxis);

	Vector3 camFwdAxis = Vector::Cross(Vector3(0, 1, 0), rightAxis);
	camFwdAxis = Vector::Normalise(camFwdAxis);

	Vector3 moveDir = Vector3(0, 0, 0);
	float  speed = 50.0f * dt;

	if (Window::GetKeyboard()->KeyDown(KeyCodes::W)) {
		moveDir = (fwdAxis);
		//selectionObject->GetPhysicsObject()->AddForce(fwdAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyCodes::S)) {
		moveDir = (-fwdAxis);
		//selectionObject->GetPhysicsObject()->AddForce(-fwdAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyCodes::A)) {
		moveDir = (-rightAxis);
		//selectionObject->GetPhysicsObject()->AddForce(-rightAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyCodes::D)) {
		moveDir = (rightAxis);
		//selectionObject->GetPhysicsObject()->AddForce(rightAxis);
	}

	playerObj->GetPhysicsObject()->AddForce(moveDir * speed);

	if (Window::GetKeyboard()->KeyDown(KeyCodes::SPACE) && playerGroundCollision->getIsCollided() == true) {
		playerObj->GetPhysicsObject()->ApplyLinearImpulse(Vector3(0, 2.0f, 0) * dt);
	}

	playerGroundCollision->GetPhysicsObject()->SetLinearVelocity(playerObj->GetPhysicsObject()->GetLinearVelocity());

}



void TutorialGame::InitWorld() {
	world.ClearAndErase();
	physics.Clear();
	initFPSTest();
	//initGame();
}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
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

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple'
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, bool isRendered, float inverseMass,
	bool isTrigger, int collisionLayer) {
	GameObject* sphere = new GameObject();

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(isTrigger, radius);
	sphere->SetBoundingVolume(volume);
	volume->collisionLayer = collisionLayer;
	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	if (isRendered) {
		sphere->SetRenderObject(new RenderObject(sphere->GetTransform(), sphereMesh, checkerMaterial));
	}

	sphere->SetPhysicsObject(new PhysicsObject(sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();

	world.AddGameObject(sphere);

	return sphere;
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

playerObject* TutorialGame::AddPlayerToWorld(const Vector3& position, Rendering::Mesh* characterMesh,
	const float scale, bool isTrigger, int collisionLayer) {
	float meshSize = scale;
	float inverseMass = 50.0f;

	playerObject* character = new playerObject();
	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize, isTrigger);
	character->SetBoundingVolume(volume);
	volume->collisionLayer = collisionLayer;

	// If mesh pivot is at feet, lower the transform so the mesh center aligns with AABB center
	const float halfHeight = 0.9f * meshSize;
	const Vector3 visualPivotOffset(0.0f, -halfHeight, 0.0f);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	if (isTrigger) {
		character->setIsCollided(false);
	}
	else {
		character->setIsCollided(true);
	}

	//character->SetRenderObject(new RenderObject(character->GetTransform(), characterMesh, notexMaterial));
	character->SetPhysicsObject(new PhysicsObject(character->GetTransform(), character->GetBoundingVolume()));
	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitCubeInertia();

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position + visualPivotOffset);

	character->SetRenderObject(new RenderObject(character->GetTransform(), characterMesh, notexMaterial));

	world.AddGameObject(character);

	return character;
}

EnemyObject* TutorialGame::AddEnemyToWorld(const Vector3& position, Rendering::Mesh* characterMesh,
    float scale, bool isTrigger, int collisionLayer) {
    float meshSize = scale;
    float inverseMass = 0.5f;

    EnemyObject* character = new EnemyObject(data, world);
    character->setPlayer(playerObj);

    AABBVolume* volume = new AABBVolume(Vector3(1.0f, 0.9f, 1.0f) * meshSize, isTrigger);
    character->SetBoundingVolume(volume);
    volume->collisionLayer = collisionLayer;

    character->GetTransform()
        .SetScale(Vector3(meshSize, meshSize, meshSize))
        .SetPosition(position);

    character->setIsCollided(!isTrigger);

    character->SetRenderObject(new RenderObject(character->GetTransform(), characterMesh, notexMaterial));
    character->SetPhysicsObject(new PhysicsObject(character->GetTransform(), character->GetBoundingVolume()));
    character->GetPhysicsObject()->SetInverseMass(inverseMass);
    character->GetPhysicsObject()->InitSphereInertia();

    world.AddGameObject(character);

    OnEnemySpawned(*character);

    return character;
}

GameObject* TutorialGame::AddBonusToWorld(const Vector3& position, Rendering::Mesh* characterMesh,
	float scale, bool isTrigger, int collisionLayer) {
	GameObject* apple = new GameObject();

	SphereVolume* volume = new SphereVolume(0.5f);
	volume->collisionLayer = collisionLayer;
	apple->SetBoundingVolume(volume);
	//volume->collisionLayer = collisionLayer;
	apple->GetTransform()
		.SetScale(Vector3(scale, scale, scale))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(apple->GetTransform(), characterMesh, glassMaterial));
	apple->SetPhysicsObject(new PhysicsObject(apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world.AddGameObject(apple);

	return apple;
}

pickUpObject* NCL::CSC8503::TutorialGame::AddPickupToWorld(const NCL::Maths::Vector3& position, Rendering::Mesh* pickupMesh,
	const float scale, int type, bool isTrigger, int collisionLayer)
{
	pickUpObject* pickObj = new pickUpObject(type);
	AABBVolume* volume = new AABBVolume(Vector3(scale, scale, scale), isTrigger);
	volume->collisionLayer = collisionLayer;

	pickObj->SetBoundingVolume(volume);
	pickObj->GetTransform()
		.SetScale(Vector3(scale, scale, scale))
		.SetPosition(position);

	pickObj->setIsCollided(isTrigger ? false : true);

	pickObj->SetRenderObject(new RenderObject(pickObj->GetTransform(), pickupMesh, checkerMaterial));
	//pickObj->setIsRender(true);
	pickObj->GetRenderObject()->SetColour(pickObj->getColour());
	pickObj->SetPhysicsObject(new PhysicsObject(pickObj->GetTransform(), pickObj->GetBoundingVolume()));

	pickObj->GetPhysicsObject()->SetInverseMass(0.0f);
	pickObj->GetPhysicsObject()->InitCubeInertia();

	pickObj->setIsCollided(false);

	world.AddGameObject(pickObj);
	return pickObj;
}

triggerObject* NCL::CSC8503::TutorialGame::addTriggerVolume(const NCL::Maths::Vector3& position, const float scaleX,
	const float scaleY, const float scaleZ, const Vector3& trigHalfDims, int collisionLayer)
{
	triggerObject* trigObj = new triggerObject();

	AABBVolume* trigVolume = new AABBVolume(trigHalfDims, true);
	trigVolume->collisionLayer = collisionLayer;
	trigObj->SetBoundingVolume(trigVolume);
	trigObj->GetTransform().
		SetScale(Vector3(scaleX, scaleY, scaleZ)).
		SetPosition(position);

	trigObj->SetPhysicsObject(new PhysicsObject(trigObj->GetTransform(), trigObj->GetBoundingVolume()));

	trigObj->GetPhysicsObject()->SetInverseMass(0.0f);
	trigObj->GetPhysicsObject()->InitCubeInertia();

	world.AddGameObject(trigObj);
	return trigObj;
}

GameObject* NCL::CSC8503::TutorialGame::addWall(const NCL::Maths::Vector3& position, Vector3& halfDims, float inverseMass,
	int collisionLayer)
{
	GameObject* wallObj = new GameObject();
	AABBVolume* wallVol = new AABBVolume(halfDims);
	wallVol->collisionLayer = collisionLayer;
	wallObj->SetBoundingVolume(wallVol);
	wallObj->GetTransform()
		.SetScale(halfDims * 2.0f)
		.SetPosition(position);
	wallObj->SetRenderObject(new RenderObject(wallObj->GetTransform(), cubeMesh, checkerMaterial));
	wallObj->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
	wallObj->SetPhysicsObject(new PhysicsObject(wallObj->GetTransform(), wallObj->GetBoundingVolume()));

	wallObj->GetPhysicsObject()->SetInverseMass(inverseMass);
	wallObj->GetPhysicsObject()->InitCubeInertia();

	world.AddGameObject(wallObj);
	return wallObj;
}

GameObject* NCL::CSC8503::TutorialGame::addDropOffZone(const NCL::Maths::Vector3& position, Vector3& halfDims, bool isTrigger,
	int collisionLayer)
{
	GameObject* dropZone = new GameObject();
	AABBVolume* dropVol = new AABBVolume(halfDims, isTrigger);
	dropVol->collisionLayer = collisionLayer;
	dropZone->SetBoundingVolume(dropVol);
	dropZone->GetTransform()
		.SetScale(halfDims * 2.0f)
		.SetPosition(position);
	dropZone->SetRenderObject(new RenderObject(dropZone->GetTransform(), cubeMesh, glassMaterial));
	dropZone->SetPhysicsObject(new PhysicsObject(dropZone->GetTransform(), dropZone->GetBoundingVolume()));

	dropZone->GetPhysicsObject()->SetInverseMass(0);
	dropZone->GetPhysicsObject()->InitCubeInertia();

	world.AddGameObject(dropZone);
	return dropZone;
}

StateGameObject* TutorialGame::AddStateObjectToWorld(const Vector3& position, Rendering::Mesh* characterMesh,
	float scale)
{
	StateGameObject* stateObj = new StateGameObject();
	SphereVolume* volume = new SphereVolume(0.5f);
	stateObj->SetBoundingVolume(volume);
	stateObj->GetTransform()
		.SetScale(Vector3(scale, scale, scale))
		.SetPosition(position);

	stateObj->SetRenderObject(new RenderObject(stateObj->GetTransform(), characterMesh, glassMaterial));
	stateObj->SetPhysicsObject(new PhysicsObject(stateObj->GetTransform(), stateObj->GetBoundingVolume()));

	stateObj->GetPhysicsObject()->SetInverseMass(0.2f);
	stateObj->GetPhysicsObject()->InitSphereInertia();

	world.AddGameObject(stateObj);

	return stateObj;
}

void TutorialGame::InitGameExamples() {
	CreatedMixedGrid(15, 15, 3.5f, 3.5f);
	playerObj = AddPlayerToWorld(Vector3(0, 5, 0), catMesh, 1.0f);
	AddEnemyToWorld(Vector3(5, 5, 0), enemyMesh, 3.0f);
	AddBonusToWorld(Vector3(10, 5, 0), bonusMesh, 2.0);
	BridgeConstraintTest();
	testStateObject = AddStateObjectToWorld(Vector3(20, 10, -20), bonusMesh, 2.0);
}

void NCL::CSC8503::TutorialGame::InitTriggerTest()
{
	playerObj = AddPlayerToWorld(Vector3(5, -11.5, 0), enemyMesh, 3.0f);
	playerGroundCollision = AddSphereToWorld(playerObj->GetTransform().GetPosition(), 0.5f, false, 0.1f, false,
		playerColliderLayer);
	pickUp = AddPickupToWorld(Vector3(10, -15, 0), cubeMesh, 1.0f, 1);
	pickUp = AddPickupToWorld(Vector3(15, -15, 0), cubeMesh, 1.0f, 1);
	AddFloorToWorld(Vector3(0, -20, 0), 50, 50);

	//AddSphereToWorld(Vector3(10, -10, 0), 0.5f, true);
}

void NCL::CSC8503::TutorialGame::initAITest()
{
	data = levelCreate();
	enemyAI = AddEnemyToWorld(Vector3(60, -7, 120), enemyMesh, 3.0f);
}

void NCL::CSC8503::TutorialGame::initObstacleTest()
{
	playerObj = AddPlayerToWorld(Vector3(0.18, -11.5, 18.72), enemyMesh, 3.0f);
	playerGroundCollision = AddSphereToWorld(playerObj->GetTransform().GetPosition(), 0.5f, false, 0.1f, false,
		playerColliderLayer);
	Vector3 penPos = Vector3(10, 10, 10);
	pendulum = pendulumConstraint(penPos, 10, 2);
	AddFloorToWorld(Vector3(0, -20, 0), 50, 50);
}

void NCL::CSC8503::TutorialGame::initGame()
{
	levelItems.clear();
	gameTime = 60.0f * 3;
	data = levelCreate();
	/*for (int i = 0; i < 4; ++i) {
		enemies.reserve(4);
		enemies.emplace_back(AddEnemyToWorld(Vector3(60, -7, 60), enemyMesh, 3.0f)
	}*/
	enemyAI = AddEnemyToWorld(Vector3(60, -7, 60), enemyMesh, 3.0f);
	//AddEnemyToWorld(Vector3(60 + 10, -7, 60), enemyMesh, 3.0f);
}

void NCL::CSC8503::TutorialGame::initFPSTest()
{
	world.Clear();
	physics.Clear();
	AddFloorToWorld(Vector3(0, -5, 0), 10000, 10000);
	CreateAABBGrid(50, 50, 5.0f, 5.0f, Vector3(1, 1, 1));
	
}

void TutorialGame::CreateSphereGrid(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
	//AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::CreatedMixedGrid(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
			}
			else {
				AddSphereToWorld(position, sphereRadius, true);
			}
		}
	}
}

void TutorialGame::CreateAABBGrid(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols + 1; ++x) {
		for (int z = 1; z < numRows + 1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
}

/*
Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around.

*/
bool TutorialGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		Debug::Print("Press Q to change to camera mode!", Vector2(5, 85));

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::Left)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
				selectionObject = nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(world.GetMainCamera());

			RayCollision closestCollision;
			if (world.Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;

				selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
				return true;
			}
			else {
				return false;
			}
		}
		if (Window::GetKeyboard()->KeyPressed(NCL::KeyCodes::L)) {
			if (selectionObject) {
				if (lockedObject == selectionObject) {
					lockedObject = nullptr;
				}
				else {
					lockedObject = selectionObject;
				}
			}
		}
	}
	else {
		Debug::Print("Press Q to change to select mode!", Vector2(5, 85));
	}
	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/

void TutorialGame::MoveSelectedObject() {
	Debug::Print("Click Force:" + std::to_string(forceMagnitude), Vector2(10, 20));
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 100.0f;

	if (!selectionObject) {
		return;//we haven't selected anything!
	}
	//Push the selected object!
	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::Right)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(world.GetMainCamera());

		RayCollision closestCollision;
		if (world.Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
			}
		}
	}
}

void TutorialGame::LockedObjectMovement() {
	Matrix4 view = world.GetMainCamera().BuildViewMatrix();
	Matrix4 camWorld = Matrix::Inverse(view);

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis = Vector::Normalise(fwdAxis);

	if (Window::GetKeyboard()->KeyDown(KeyCodes::UP)) {
		selectionObject->GetPhysicsObject()->AddForce(fwdAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyCodes::DOWN)) {
		selectionObject->GetPhysicsObject()->AddForce(-fwdAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyCodes::NEXT)) {
		selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
	}
}

void NCL::CSC8503::TutorialGame::BridgeConstraintTest()
{
	Vector3 cubeSize = Vector3(8, 8, 8);

	float invCubeMass = 5;//How heavy the middle pieces are
	int numLinks = 10;
	float maxDistance = 30;//constraint distance
	float cubeDistance = 20;//distance between links

	Vector3 startPos = Vector3(50, 50, 50);

	GameObject* start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0);
	GameObject* end = AddCubeToWorld(startPos + Vector3((numLinks + 2) * cubeDistance, 0, 0),
		cubeSize, 0);

	GameObject* previous = start;

	for (int i = 0; i < numLinks; ++i) {
		GameObject* block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0),
			cubeSize, invCubeMass);
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance);
		world.AddConstraint(constraint);
		previous = block;
	}
	PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance);
	world.AddConstraint(constraint);
}

obstacleObject* NCL::CSC8503::TutorialGame::pendulumConstraint(const Vector3& anchorPos, int numLinks, float linkLength)
{
	GameObject* anchor = AddCubeToWorld(anchorPos, Vector3(0.5f, 0.5f, 0.5f), 0.0f, true, terrainLayer);

	const Vector3 swingAxis = Vector3(0, 0, 1);

	GameObject* prev = anchor;
	for (int i = 0; i < numLinks; ++i) {
		Vector3 linkPos = anchorPos + Vector3(0.0f, -(i + 1) * linkLength, 0.0f);
		GameObject* link = AddCubeToWorld(linkPos, Vector3(0.25f, 0.25, 0.25f), 1.0f, true, defaultLayer);

		world.AddConstraint(new PositionConstraint(prev, link, linkLength));
		world.AddConstraint(new OrientationConstraint(prev, link, swingAxis));

		prev = link;
	}

	obstacleObject* bob = new obstacleObject();
	SphereVolume* bobVol = new SphereVolume(false, 2.0f);
	bobVol->collisionLayer = defaultLayer;
	bob->SetBoundingVolume(bobVol);
	bob->GetTransform()
		.SetScale(Vector3(2.0f, 2.0f, 2.0f))
		.SetPosition(anchorPos + Vector3(0, -(numLinks + 1) * linkLength, 0.0f));
	bob->SetRenderObject(new RenderObject(bob->GetTransform(), sphereMesh, checkerMaterial));
	bob->SetPhysicsObject(new PhysicsObject(bob->GetTransform(), bob->GetBoundingVolume()));
	bob->GetPhysicsObject()->SetInverseMass(1.0f);
	bob->GetPhysicsObject()->InitSphereInertia();

	world.AddGameObject(bob);

	world.AddConstraint(new PositionConstraint(prev, bob, linkLength));
	world.AddConstraint(new OrientationConstraint(prev, bob, swingAxis));

	bob->GetPhysicsObject()->ApplyLinearImpulse(Vector3(100.0f, 0, 0));
	bob->GetPhysicsObject()->ApplyAngularImpulse(swingAxis * 5.0f);

	Vector3 bobPos = bob->GetTransform().GetPosition();

	trigVol = addTriggerVolume(bobPos + Vector3(10, 0, 0), 2.5, 2.5, 2.5, Vector3(2.5, 2.5, 2.5));

	return bob;
}

levelElements* NCL::CSC8503::TutorialGame::levelCreate()
{
	levelElements* level = new levelElements("level.txt");
	int nodeSize = level->getNodeSize();
	int gridWidth = level->getLevelWidth();
	int gridHeight = level->getLevelHeight();
	levelNode* nodes = level->getAllLevelNodes();
	float cubeHeight = nodeSize * 0.25f;

	for (int i = 0; i < gridWidth * gridHeight; ++i) {
		levelNode& lNodes = nodes[i];
		int type = lNodes.type;
		int obstacleNum = 0;
		int itemNum = 0;
		if (isdigit(type)) {
			float unitHeight = cubeHeight * (float(type) - 48);
			AddCubeToWorld(lNodes.position - Vector3(0, unitHeight + 8, 0), Vector3(nodeSize / 2, nodeSize / 2, nodeSize / 2), 0.0f);
		}
		if (type == 'P') {
			lNodes.position.y = 0;
			playerObj = AddPlayerToWorld(lNodes.position - Vector3(0, 10, 0), playerMesh, 3.0f);
			playerObj->setRespawn(lNodes.position - Vector3(0, 10, 0));
			playerGroundCollision = AddSphereToWorld(playerObj->GetTransform().GetPosition(), 0.5f, false, 0.1f, false,
				playerColliderLayer);
		}
		if (type == 'W') {
			lNodes.position.y = 0;
			Vector3 wallHalfDims = Vector3(nodeSize / 2, nodeSize / 2, nodeSize / 2);
			movableWall = addWall(lNodes.position - Vector3(0, (nodeSize / 2) + 8, 0), wallHalfDims);
		}
		if (type == 'D') {
			lNodes.position.y = 0;
			Vector3 zoneHalfDims = Vector3(nodeSize / 2, nodeSize / 2, nodeSize / 2);
			dropOffZone = addDropOffZone(lNodes.position - Vector3(0, (nodeSize / 2) + 8, 0), zoneHalfDims);
		}
		if (type == 'I') {
			itemNum++;
			levelItems.reserve(itemNum);
			lNodes.position.y = 0;
			levelItems.emplace_back(AddPickupToWorld(lNodes.position - Vector3(0, (nodeSize / 2) + 10, 0), cubeMesh, 1));
		}
		if (type == 'B') {
			itemNum++;
			levelItems.reserve(itemNum);
			lNodes.position.y = 0;
			levelItems.emplace_back(AddPickupToWorld(lNodes.position - Vector3(0, (nodeSize / 2) + 10, 0), cubeMesh, 1, 1));
		}
		if (type == 'O') {
			obstacleNum++;
			obstacles.resize(obstacleNum);
			lNodes.position.y = 10;
			obstacles.emplace_back(pendulumConstraint(lNodes.position, 10, 2));
		}
	}
	const float gridWorldWidth = (float)(gridWidth * nodeSize);
	const float gridWorldHeight = (float)(gridHeight * nodeSize);

	const Vector3 floorCenter(
		gridWorldWidth * 0.5f,
		-20, 
		gridWorldHeight * 0.5f
	);

	const float floorHalfX = gridWorldWidth * 0.55;
	const float floorHalfZ = gridWorldHeight * 0.55;
	const float floorHalfY = 1.0f; 

	levelFloor = AddFloorToWorld(floorCenter, floorHalfX, floorHalfZ);
	//outOfBounds = addTriggerVolume(floorCenter, 100, 100, 100, Vector3(100, 100, 100), outOfBoundLayer);
	return level;
}

void TutorialGame::DebugObjectMovement() {
	//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyCodes::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
	}
}