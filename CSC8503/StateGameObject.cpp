#include "StateGameObject.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"
#include "PhysicsObject.h"
#include "CollisionDetection.h"
#include "CollisionVolume.h"
#include "Debug.h"

using namespace NCL;
using namespace CSC8503;

StateGameObject::StateGameObject() {
	counter = 0.0f;
	stateMachine = new StateMachine();

	State* stateA = new State([&](float dt)->void {
		this->MoveLeft(dt);
		}
	);

	State* stateB = new State([&](float dt)->void {
		this->MoveRight(dt);
		}
	);

	stateMachine->AddState(stateA);
	stateMachine->AddState(stateB);

	stateMachine->AddTransition(new StateTransition(stateA, stateB, [&]()-> bool {
		return this->counter > 3.0f;
		}
	));

	stateMachine->AddTransition(new StateTransition(stateB, stateA, [&]()-> bool {
		return this->counter < 0.0f;
		}
	));
}

StateGameObject::~StateGameObject() {
	delete stateMachine;
}

void StateGameObject::Update(float dt) {
	stateMachine->Update(dt);
}

void StateGameObject::MoveLeft(float dt) {
	GetPhysicsObject()->AddForce({ -100, 0, 0 });
	counter += dt;
}

void StateGameObject::MoveRight(float dt) {
	GetPhysicsObject()->AddForce({ 100, 0, 0 });
	counter -= dt;
}

NCL::CSC8503::EnemyObject::EnemyObject(levelElements* level, GameWorld& game) :
	StateGameObject(), gameWorld(game)
{
	data = level;
	gameWorld = game;
	enemyStateMachine = new StateMachine();
	targetPosition = this->GetTransform().GetPosition();
	if (data) {
		navigationGridFile = level->getNavFile();
	}

	State* chaseState = new State([&](float dt)->void {
		this->chasePlayer(dt);
	});

	State* wanderState = new State([&](float dt)-> void {
		this->wander(dt);
	});

	StateTransition* wanderToChase = new StateTransition(wanderState, chaseState, [&](void)->bool {
		moveSpeed = chaseSpeed;
		return canSeePlayer();
	});

	StateTransition* chaseToWander = new StateTransition(chaseState, wanderState, [&](void)->bool {
		return !canSeePlayer();
	});

	enemyStateMachine->AddState(wanderState);
	enemyStateMachine->AddState(chaseState);
	enemyStateMachine->AddTransition(wanderToChase);
	enemyStateMachine->AddTransition(chaseToWander);
}

NCL::CSC8503::EnemyObject::~EnemyObject()
{
	delete enemyStateMachine;
}

void NCL::CSC8503::EnemyObject::setWalkingPoints()
{
	pathFindingNodes.clear();
	pathFindingNodes.reserve(data->getWalkable().size());
	NavigationGrid navGrid(navigationGridFile);
	NavigationPath outPath;

	Vector3 startPos = GetTransform().GetPosition();
	startPos.y = 0;
	Vector3 endPos = targetPosition;

	bool found = navGrid.FindPath(startPos, endPos, outPath);
	if (!found) {
		return;
	}

	Vector3 position;
	while (outPath.PopWaypoint(position)) {
		pathFindingNodes.push_back(position);
	}
}

void NCL::CSC8503::EnemyObject::drawWalkingPoints()
{
	for (int i = 1; i < pathFindingNodes.size(); ++i) {
		Vector3 a = pathFindingNodes[i - 1];
		Vector3 b = pathFindingNodes[i];
		Debug::debugDrawSphere(a, 2.0f, Vector4(0, 1, 0, 1), 0.1f, 16);
		Debug::debugDrawSphere(b, 2.0f, Vector4(0, 1, 0, 1), 0.1f, 16);
		Debug::DrawLine(a, b, Vector4(0, 0, 1, 1));
	}
}

void NCL::CSC8503::EnemyObject::moveEnemy()
{
	Vector3 dir = Vector::Normalise(targetPosition - this->GetTransform().GetPosition());
	this->GetPhysicsObject()->AddForce(dir * moveSpeed);
	//std::cout << "I don't see you";
}

void NCL::CSC8503::EnemyObject::Update(float dt)
{
	enemyStateMachine->Update(dt);
}

void NCL::CSC8503::EnemyObject::OnCollisionBegin(GameObject* other)
{
	if (other->GetBoundingVolume()->collisionLayer == NCL::playerLayer) {
		std::cout << "hit\n";
		this->GetPhysicsObject()->ApplyLinearImpulse(Vector3(100, 0, 0));
	}
}

void NCL::CSC8503::EnemyObject::chasePlayer(float dt)
{
	//std::cout << "I see you\n";
	targetPosition = player->GetTransform().GetPosition();
	setWalkingPoints();
	drawWalkingPoints();
	moveEnemy();
	//std::cout << "I can see you\n";
}

void NCL::CSC8503::EnemyObject::wander(float dt)
{
	if (data) {
		std::cout << spotDuration << "\n";
		std::cout << foundSpot << "\n";
		std::cout << searchingForNextSpot << "\n";
		if (Vector::Length(this->GetTransform().GetPosition() - targetPosition) < data->getNodeSize() || spotDuration >= 20.0f) {
			searchingForNextSpot = true;
			//setWalkingPoints();
		}

		if (searchingForNextSpot) {
			targetPosition = data->getWalkable()[RandomValue(0, data->getWalkable().size() - 1)].position;
			targetPosition.y = 0;
			spotDuration = 0.0f;
			searchingForNextSpot = false;
		}

		if (!searchingForNextSpot) {
			//std::cout << "found spot\n";
			setWalkingPoints();
			moveEnemy();
		}
		drawWalkingPoints();
		spotDuration += dt;
	}
}

bool NCL::CSC8503::EnemyObject::canSeePlayer()
{
	std::vector<int>ignoreList;
	ignoreList.reserve(8);
	int enemyLayer = this->GetBoundingVolume()->collisionLayer;
	ignoreList.emplace_back(enemyLayer);
	if (player) {
		Vector3 origin = this->GetTransform().GetPosition();
		Vector3 end = Vector::Normalise(player->GetTransform().GetPosition() - this->GetTransform().GetPosition());
		Ray ray(origin, end);
		RayCollision closestCollision;
		//Debug::DrawLine(origin, origin + forward * Vector3(0, 0, 200), Vector4(0, 0, 1, 1), 0.1f);
		if (gameWorld.Raycast(ray, closestCollision, true, this)) {
			Debug::DrawLine(origin, closestCollision.collidedAt, Vector4(0, 0, 1, 1), 0.1f);
			GameObject* sightedObject = (GameObject*)closestCollision.node;
			if (sightedObject->GetBoundingVolume()->collisionLayer != playerLayer) {
				ignoreList.emplace_back(sightedObject->GetBoundingVolume()->collisionLayer);
			}
			if (sightedObject->GetBoundingVolume()->collisionLayer == playerLayer) {
				return true;
			}
		}
		return false;
	}
	//std::cout << "no player\n";
	return false;
}