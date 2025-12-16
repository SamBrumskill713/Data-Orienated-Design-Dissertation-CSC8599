#pragma once
#include "GameObject.h"
#include "GameWorld.h"
#include "NavigationGrid.h"

namespace NCL {
    namespace CSC8503 {
        class StateMachine;
        class StateGameObject : public GameObject  
        {
        public:
            StateGameObject();
            ~StateGameObject();

            virtual void Update(float dt);

        protected:
            void MoveLeft(float dt);
            void MoveRight(float dt);

            StateMachine* stateMachine;
            float counter;
        };

        class EnemyObject : public StateGameObject {
        public:
            EnemyObject(levelElements* level, GameWorld& world);
            ~EnemyObject();
            void chasePlayer(float dt);
            void wander(float dt);
            bool canSeePlayer();
            void setPlayer(playerObject* player) {
                this->player = player;
            }
            void setWalkingPoints();
            void drawWalkingPoints();
            void moveEnemy();
            void Update(float dt) override;
            void OnCollisionBegin(GameObject* other) override;

        protected:
            StateMachine* enemyStateMachine;
            float moveSpeed = 10.0f;
            float chaseSpeed = 20.0f;
            GameWorld& gameWorld;
            levelElements* data;
            playerObject* player;
            Vector3 targetPosition;
            std::string navigationGridFile;
            bool searchingForNextSpot = true;
            bool foundSpot = false;
            float spotDuration = 0.0f;
            std::vector<Vector3> pathFindingNodes;
        };
    }
}
