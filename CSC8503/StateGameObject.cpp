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
	State* chaseState = new State([&](float dt)->void {
		this->chasePlayer();
	});

	State* wanderState = new State([&](float dt)-> void {
		this->wander();
	});

	StateTransition* wanderToChase = new StateTransition(wanderState, chaseState, [&](void)->bool {
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

void NCL::CSC8503::EnemyObject::Update(float dt)
{
	enemyStateMachine->Update(dt);
}

void NCL::CSC8503::EnemyObject::chasePlayer()
{
	std::cout << "I can see you\n";
}

void NCL::CSC8503::EnemyObject::wander()
{
	std::cout << "I don't see you\n";
}

bool NCL::CSC8503::EnemyObject::canSeePlayer()
{
	std::vector<int>ignoreList;
	int enemyLayer = this->GetBoundingVolume()->collisionLayer;
	ignoreList.emplace_back(enemyLayer);
	if (player) {
		Vector3 origin = this->GetTransform().GetPosition();
		Vector3 forward = this->GetTransform().GetOrientation() * Vector3(0, 0, -1);
		forward.y = 0.0f; // keep ray in horizontal plane if desired
		forward = Vector::Normalise(forward);
		Ray ray(origin, forward);
		RayCollision closestCollision;
		if (gameWorld.Raycast(ray, closestCollision, true, this)) {
			GameObject* sightedObject = (GameObject*)closestCollision.node;
			Debug::DrawLine(origin, origin + forward * Vector3(0, 0, 200), Vector4(0, 0, 1, 1), 0.1f);
			if (sightedObject->GetBoundingVolume()->collisionLayer == playerLayer) {
				return true;
			}
		}
		return false;
	}
	std::cout << "no player\n";
	return false;
}
