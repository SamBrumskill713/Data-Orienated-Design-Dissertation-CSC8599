#pragma once
#include "GameObject.h"

namespace NCL {
    namespace CSC8503 {
        class StateMachine;
        class StateGameObject : public GameObject  
        {
        public:
            StateGameObject();
            ~StateGameObject();

            virtual void Update(float dt);

            StateGameObject* AddStateObjectToWorld(const Vector3& position);
            StateGameObject* testStateObject;

        protected:
            void MoveLeft(float dt);
            void MoveRight(float dt);

            StateMachine* stateMachine;
            float counter;
        };

        class EnemyObject : public StateGameObject {
        public:
            EnemyObject();
            ~EnemyObject();

        protected:
            float moveSpeed = 10.0f;

        };
    }
}
