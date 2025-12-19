#include "NetworkedGame.h"
#include "NetworkPlayer.h"
#include "NetworkObject.h"
#include "GameServer.h"
#include "GameClient.h"
#include "GameWorld.h"
#include "Window.h"
#include "Debug.h"
#include "PhysicsObject.h"
#include "StateGameObject.h"

#include <functional>
#include <limits>

#define COLLISION_MSG 30
#define OWNERSHIP_MSG 201

using namespace NCL;
using namespace CSC8503;

static int gBroadcastCount = 0;
static int gLastAckFromPeer = -1;
static int gLastAckStateId = 0;

static std::unordered_map<int, struct SmoothPos> gSmoothPos;
struct SmoothPos {
	Vector3 from;
	Vector3 to;
	float   t = 0.0f;
	float   duration = 1.0f / 120.0f;
};


static std::unordered_map<int, struct PredError> gPredErrors;
struct PredError {
	Vector3 posPending;
	Vector3 velPending;
};

static std::unordered_map<int, struct SmoothOri> gSmoothOri;
struct SmoothOri {
	Quaternion from;
	Quaternion to;
	float t = 0.0f;
	float duration = 1.0f / 120.0f;
};


static inline Quaternion FacingFromYawDeg(float pitch, float yawDeg) {
	return Quaternion::EulerAnglesToQuaternion(pitch, yawDeg, 0.0f);
}

struct MessagePacket : public GamePacket {
	short playerID;
	short messageID;

	MessagePacket() {
		type = Message;
		size = sizeof(short) * 2;
	}
};

struct AcknowledgePacket : public GamePacket {
	int lastID;
	AcknowledgePacket(int id = 0) : GamePacket(Received_State), lastID(id) {
		size = sizeof(int);
	}
};


struct ClientInputPacket : public GamePacket {
	int            seq;       
	unsigned char  buttons;   
	unsigned char  reserved[3]; 
	float          pitch;     
	float          yaw;       
	ClientInputPacket() : GamePacket(Message), seq(0), buttons(0), reserved{0,0,0}, pitch(0.0f), yaw(0.0f) {
		size = static_cast<short>(sizeof(ClientInputPacket)); 
	}
};

struct OwnershipPacket : public GamePacket {
	int objectID;
	OwnershipPacket(int id = -1) : GamePacket(OWNERSHIP_MSG), objectID(id) {
		size = sizeof(int);
	}
};

NetworkedGame::NetworkedGame(GameWorld& gameWorld, GameTechRendererInterface& renderer, PhysicsSystem& physics) :
	TutorialGame(gameWorld, renderer, physics)
{
	thisServer = nullptr;
	thisClient = nullptr;

	NetworkBase::Initialise();
	timeToNextPacket = 0.0f;
	packetsToSnapshot = 0;

	allowLocalPlayerControl = thisServer != nullptr;

	localPlayer = nullptr;
}

NetworkedGame::~NetworkedGame() {
	delete thisServer;
	delete thisClient;
}

void NetworkedGame::StartAsServer() {
	thisServer = new GameServer(NetworkBase::GetDefaultPort(), 4);

	thisServer->RegisterPacketHandler(Received_State, this);
	thisServer->RegisterPacketHandler(Player_Connected, this);
	thisServer->RegisterPacketHandler(Player_Disconnected, this);
	thisServer->RegisterPacketHandler(Message, this);

	thisServer->SetGameWorld(world);

	SetLocalPlayerControl(true); 

	std::cout << "[Server] Started on port " << NetworkBase::GetDefaultPort() << " (max clients: 4)" << std::endl;

	StartLevel();

	localPlayer = playerObj;
	SetCameraTarget(localPlayer);
}

void NetworkedGame::StartAsClient(char a, char b, char c, char d) {
    thisClient = new GameClient();
    thisClient->Connect(a, b, c, d, NetworkBase::GetDefaultPort());

    thisClient->RegisterPacketHandler(Delta_State, this);
    thisClient->RegisterPacketHandler(Full_State, this);
    thisClient->RegisterPacketHandler(Player_Connected, this);
    thisClient->RegisterPacketHandler(Player_Disconnected, this);
    thisClient->RegisterPacketHandler(OWNERSHIP_MSG, this);

    SetLocalPlayerControl(false);
    StartLevel();

    {
        std::vector<GameObject*> toRemove;
        std::vector<GameObject*>::const_iterator first, last;
        world.GetObjectIterators(first, last);
        for (auto it = first; it != last; ++it) {
            if (auto* enemy = dynamic_cast<EnemyObject*>(*it)) {
                toRemove.push_back(enemy);
            }
        }
        for (auto* e : toRemove) {
            world.RemoveGameObject(e, true);
        }
    }

    localPlayer = nullptr;
    SetCameraTarget(nullptr);
}

void NetworkedGame::UpdateGame(float dt) {
	timeToNextPacket -= dt;
	if (timeToNextPacket < 0) {
		if (thisServer) {
			UpdateAsServer(dt);
		}
		else if (thisClient) {
			UpdateAsClient(dt);
		}
		timeToNextPacket += 1.0f / 120.0f; //20hz server/client update
	}

	if (!thisServer && Window::GetKeyboard()->KeyPressed(KeyCodes::F9)) {
		StartAsServer();
	}
	if (!thisClient && Window::GetKeyboard()->KeyPressed(KeyCodes::F10)) {
		StartAsClient(127, 0, 0, 1);
	}

	//if (thisServer) {
	//	Debug::Print("Server: Snapshots sent = " + std::to_string(gBroadcastCount), Vector2(0, 25), Debug::WHITE);
	//	if (gLastAckFromPeer >= 0) {
	//		Debug::Print("Last Ack: peer=" + std::to_string(gLastAckFromPeer) +
	//			" stateID=" + std::to_string(gLastAckStateId),
	//			Vector2(5, 21), Debug::YELLOW);
	//	}
	//}
	/*if (thisClient) {
		Debug::Print("Client Game", Vector2(0, 25));
	}*/

	if (thisServer) {
		Debug::Print("Authority: Server", Vector2(0, 30), Debug::GREEN);
	}
	if (thisClient) {
		Debug::Print("Authority: Server (local control disabled)", Vector2(0, 35), Debug::YELLOW);
	}

	TutorialGame::UpdateGame(dt);
}

void NCL::CSC8503::NetworkedGame::OnEnemySpawned(EnemyObject& enemy)
{
	enemy.SetNetworkedGame(this);
}

void NetworkedGame::UpdateAsServer(float dt) {
	thisServer->UpdateServer();   
	BroadcastSnapshot(false);     
}

void NetworkedGame::UpdateAsClient(float dt) {
	thisClient->UpdateClient();

	unsigned char b = 0;
	auto* kb = Window::GetKeyboard();
	if (kb->KeyDown(KeyCodes::W)) b |= 1 << 0;
	if (kb->KeyDown(KeyCodes::A)) b |= 1 << 1;
	if (kb->KeyDown(KeyCodes::S)) b |= 1 << 2;
	if (kb->KeyDown(KeyCodes::D)) b |= 1 << 3;
	if (kb->KeyDown(KeyCodes::SPACE)) b |= 1 << 4;

	static int seq = 0;
	ClientInputPacket input;
	input.seq    = seq++;
	input.buttons= b;
	input.pitch  = world.GetMainCamera().GetPitch(); 
	input.yaw    = world.GetMainCamera().GetYaw();  
	thisClient->SendPacket(input);

	if (ownedNetId >= 0) {
		auto it = netIdToObject.find(ownedNetId);
		if (it != netIdToObject.end() && it->second) {
			GameObject* mine = it->second;
			const float yawDeg = world.GetMainCamera().GetYaw();
			const Quaternion q = Quaternion::EulerAnglesToQuaternion(0.0f, yawDeg, 0.0f);
			mine->GetTransform().SetOrientation(q);
		}
	}

	AcknowledgePacket ack(lastReceivedStateID);
	thisClient->SendPacket(ack);

	// Position interpolation
	for (auto& kv : gSmoothPos) {
		SmoothPos& s = kv.second;
		s.t += dt;
		const float a = s.duration > 0.0f ? std::min(s.t / s.duration, 1.0f) : 1.0f;
		if (auto it = netIdToObject.find(kv.first); it != netIdToObject.end() && it->second) {
			GameObject* obj = it->second;
			obj->GetTransform().SetPosition(s.from + (s.to - s.from) * a);
		}
	}

	// Orientation interpolation
	for (auto& kv : gSmoothOri) {
		if (kv.first == ownedNetId) continue;
		SmoothOri& s = kv.second;
		s.t += dt;
		const float a = s.duration > 0.0f ? std::min(s.t / s.duration, 1.0f) : 1.0f;

		if (auto it = netIdToObject.find(kv.first); it != netIdToObject.end() && it->second) {
			GameObject* obj = it->second;

			Quaternion qFrom = s.from;
			Quaternion qTo   = s.to;

			float dot = Quaternion::Dot(qFrom, qTo);
			if (dot < 0.0f) {
				qTo = -qTo;
				dot = -dot;
			}

			Quaternion q;
			if (dot > 0.9995f) {
				q = Quaternion::Lerp(qFrom, qTo, a).Normalised();
			} else {
				q = Quaternion::Slerp(qFrom, qTo, a);
			}

			obj->GetTransform().SetOrientation(q);
		}
	}

	Debug::Print("Client Game", Vector2(0, 95));
}

void NetworkedGame::BroadcastSnapshot(bool deltaFrame) {
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;
	world.GetObjectIterators(first, last);

	int packetsThisFrame = 0;

	for (auto i = first; i != last; ++i) {
		GameObject* obj = *i;
		if (!obj) continue;

		const CollisionVolume* vol = obj->GetBoundingVolume();
		if (!vol || vol->isTrigger) continue;

		const bool isPlayer = (vol->collisionLayer == playerLayer);
		const bool isEnemy  = (dynamic_cast<EnemyObject*>(obj) != nullptr);

		if (!isPlayer && !isEnemy) {
			continue;
		}

		NetworkObject* no = obj->GetNetworkObject();
		if (!no) {
			continue;
		}

		GamePacket* newPacket = nullptr;
		if (no->WritePacket(&newPacket, false, 0)) {
			thisServer->SendGlobalPacket(*newPacket);
			delete newPacket;
			++packetsThisFrame;
		}
	}
	gBroadcastCount += packetsThisFrame;
}

void NetworkedGame::UpdateMinimumState() {
	int minID = INT_MAX;
	int maxID = 0; 

	for (auto i : stateIDs) {
		minID = std::min(minID, i.second);
		maxID = std::max(maxID, i.second);
	}

	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;
	world.GetObjectIterators(first, last);

	for (auto i = first; i != last; ++i) {
		NetworkObject* o = (*i)->GetNetworkObject();
		if (!o) {
			continue;
		}
		o->UpdateStateHistory(minID);
	}
}

void NetworkedGame::StartLevel() {
	std::vector<GameObject*>::const_iterator first, last;
	world.GetObjectIterators(first, last);

	for (auto it = first; it != last; ++it) {
		GameObject* obj = *it;
		if (!obj) continue;

		const CollisionVolume* vol = obj->GetBoundingVolume();
		if (!vol || vol->isTrigger) continue;

		const bool isPlayer = (vol->collisionLayer == playerLayer);
		const bool isEnemy  = (dynamic_cast<EnemyObject*>(obj) != nullptr);

		if (isPlayer && !obj->GetNetworkObject()) {
			const int id = nextObjectId++;
			obj->SetNetworkObject(new NetworkObject(*obj, id));
			netIdToObject[id] = obj;
		}

		if (isEnemy) {
			auto* enemy = static_cast<EnemyObject*>(obj);
			enemy->SetNetworkedGame(this);

			if (thisServer && !obj->GetNetworkObject()) {
				const int id = EnemyIdBias + (nextEnemyId++);
				obj->SetNetworkObject(new NetworkObject(*obj, id));
				netIdToObject[id] = obj;
			}
		}
	}
}

void NetworkedGame::ReceivePacket(int type, GamePacket* payload, int source) {
	switch (type) {
	case Message: {
		if (thisServer && payload->size == (short)sizeof(ClientInputPacket)) {
			auto* input = reinterpret_cast<ClientInputPacket*>(payload);
			GameObject* target = nullptr;
			if (auto it = serverPlayers.find(source); it != serverPlayers.end()) {
				target = it->second;
			}
			if (!target) break;

			const float yawDeg = input->yaw;
			const Quaternion facing = Quaternion::EulerAnglesToQuaternion(0.0f, yawDeg, 0.0f);
			target->GetTransform().SetOrientation(facing);

			const Vector3 euler = target->GetTransform().GetOrientation().ToEuler();

			if (auto* phys = target->GetPhysicsObject()) {
				phys->SetAngularVelocity(Vector3(0.0f, 0.0f, 0.0f));
				const float perTick   = 1.0f / 120.0f;
				const float moveForce = 50.0f;
				const Vector3 forward = facing * Vector3(0, 0, -1);
				const Vector3 right   = facing * Vector3(1, 0,  0);

				Vector3 move(0, 0, 0);
				if (input->buttons & (1 << 0)) move += forward;   // W
				if (input->buttons & (1 << 2)) move -= forward;   // S
				if (input->buttons & (1 << 1)) move -= right;     // A
				if (input->buttons & (1 << 3)) move += right;     // D

				phys->AddForce(move * moveForce * perTick);
				if (input->buttons & (1 << 4)) {
					phys->ApplyLinearImpulse(Vector3(0, 2.0f, 0) * perTick);
				}
			}
		}
	} break;

	case Full_State:
		if (thisClient) {
			auto* fp = reinterpret_cast<FullPacket*>(payload);
			GameObject* obj = GetOrCreateProxy(fp->objectID);
			if (!obj) break;

			const Vector3     predictedPos = obj->GetTransform().GetPosition();
			const Quaternion  predictedOri = obj->GetTransform().GetOrientation();

			if (NetworkObject* no = obj->GetNetworkObject()) {
				no->ReadPacket(*payload);
				lastReceivedStateID = std::max(lastReceivedStateID, fp->fullState.stateID);
			}

			const Vector3    serverPos = obj->GetTransform().GetPosition();
			const Quaternion serverOri = obj->GetTransform().GetOrientation();

			SmoothPos sp{};
			sp.from = predictedPos;
			sp.to   = serverPos;
			sp.t    = 0.0f;
			sp.duration = (fp->objectID == ownedNetId) ? 0.0f : (1.0f / 120.0f);
			gSmoothPos[fp->objectID] = sp;

			if (fp->objectID != ownedNetId) {
				SmoothOri so{};
				so.from = predictedOri; so.to = serverOri; so.t = 0.0f; so.duration = 1.0f / 120.0f;
				gSmoothOri[fp->objectID] = so;
			}

			obj->GetTransform().SetPosition(predictedPos);
			obj->GetTransform().SetOrientation(predictedOri);

			if (!cameraTarget && fp->objectID == ownedNetId) {
				SetCameraTarget(obj);
			}
		}
		break;

	case Delta_State:
		if (thisClient) {
			auto* dp = reinterpret_cast<DeltaPacket*>(payload);
			auto it = netIdToObject.find(dp->objectID);
			if (it != netIdToObject.end()) {
				GameObject* obj = it->second;
				const Vector3    predictedPos = obj->GetTransform().GetPosition();
				const Quaternion predictedOri = obj->GetTransform().GetOrientation();

				if (NetworkObject* no = obj->GetNetworkObject()) {
					no->ReadPacket(*payload);
					lastReceivedStateID = std::max(lastReceivedStateID, dp->fullID);
				}

				const Vector3    serverPos = obj->GetTransform().GetPosition();
				const Quaternion serverOri = obj->GetTransform().GetOrientation();

				SmoothPos sp{};
				sp.from = predictedPos;
				sp.to   = serverPos;
				sp.t    = 0.0f;
				sp.duration = (dp->objectID == ownedNetId) ? 0.0f : (1.0f / 120.0f); 
				gSmoothPos[dp->objectID] = sp;

				if (dp->objectID != ownedNetId) {
					SmoothOri so{};
					so.from = predictedOri; so.to = serverOri; so.t = 0.0f; so.duration = 1.0f / 120.0f;
					gSmoothOri[dp->objectID] = so;
				}

				obj->GetTransform().SetPosition(predictedPos);
				obj->GetTransform().SetOrientation(predictedOri);
			}
		}
		break;

	case Received_State:
		if (thisServer) {
			auto* ack = reinterpret_cast<AcknowledgePacket*>(payload);
			stateIDs[source] = ack->lastID;

			gLastAckFromPeer = source;
			gLastAckStateId = ack->lastID;
			std::cout << "[Server] Ack from peer " << source << " for stateID " << ack->lastID << std::endl;
		}
		break;

	case Player_Connected:
		if (thisServer) {
			std::cout << "[Server] Player connected (peer " << source << ")\n";

			Vector3 base = playerObj ? playerObj->GetTransform().GetPosition() : Vector3(0, 5, 0);
			const float spacing = 3.0f;
			const int idx = static_cast<int>(serverPlayers.size());
			Vector3 spawn(base.x + idx * spacing, base.y, base.z);

			GameObject* p = AddPlayerToWorld(spawn, playerMesh, 3.0f, false, playerLayer);
			if (p) {
				const int id = nextObjectId++;
				p->SetNetworkObject(new NetworkObject(*p, id));
				netIdToObject[id] = p;
				serverPlayers[source] = p;

				OwnershipPacket own(id);
				thisServer->SendPacketToPeer(source, own);
			}
		}
		break;

	case Player_Disconnected:
		if (thisServer) {
			auto it = serverPlayers.find(source);
			if (it != serverPlayers.end()) {
				GameObject* p = it->second;
				serverPlayers.erase(it);
				if (p) {
					world.RemoveGameObject(p, true);
				}
			}
			std::cout << "[Server] Player removed for peer " << source << "\n";
		}
		break;

	case OWNERSHIP_MSG:
		if (thisClient) {
			auto* op = reinterpret_cast<OwnershipPacket*>(payload);
			GameObject* mine = GetOrCreateProxy(op->objectID);
			ownedNetId = op->objectID;

			SetCameraTarget(mine);
		}
		break;
	}
}

void NetworkedGame::OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b) {
	if (thisServer) { //detected a collision between players!
		MessagePacket newPacket;
		newPacket.messageID = COLLISION_MSG;
		newPacket.playerID = a->GetPlayerNum();
		thisServer->SendGlobalPacket(newPacket);

		newPacket.playerID = b->GetPlayerNum();
		thisServer->SendGlobalPacket(newPacket);
	}
}

void NetworkedGame::RegisterNetworkObject(GameObject* obj, int netId) {
	if (!obj) return;
	obj->SetNetworkObject(new NetworkObject(*obj, netId));
	netIdToObject[netId] = obj;
}

GameObject* NetworkedGame::GetOrCreateProxy(int objectID) {
    auto it = netIdToObject.find(objectID);
    if (it != netIdToObject.end()) {
        return it->second;
    }

    const bool isEnemyId = (objectID >= EnemyIdBias);

    if (isEnemyId) {
        EnemyObject* proxy = AddEnemyToWorld(Vector3(), enemyMesh, 3.0f, true, enemyLayer);
        if (!proxy) return nullptr;

        if (auto* phys = proxy->GetPhysicsObject()) {
            phys->SetInverseMass(0.0f);
            phys->SetLinearVelocity(Vector3());
            phys->SetAngularVelocity(Vector3());
        }

        proxy->SetNetworkedGame(this);

        proxy->SetNetworkObject(new NetworkObject(*proxy, objectID));
        netIdToObject[objectID] = proxy;
        return proxy;
    }

    playerObject* proxy = AddPlayerToWorld(Vector3(), playerMesh, 3.0f, true, playerLayer);
    if (!proxy) return nullptr;

    if (auto* phys = proxy->GetPhysicsObject()) {
        phys->SetInverseMass(0.0f);          
        phys->SetLinearVelocity(Vector3());
        phys->SetAngularVelocity(Vector3());
    }

    proxy->SetNetworkObject(new NetworkObject(*proxy, objectID));
    netIdToObject[objectID] = proxy;
    return proxy;
}

void NetworkedGame::ForEachServerPlayer(const std::function<void(GameObject*)>& fn) const {
	if (playerObj) {
		fn(playerObj);
	}

	for (const auto& kv : serverPlayers) {
		GameObject* p = kv.second;
		if (p) {
			fn(p);
		}
	}
}

GameObject* NetworkedGame::FindClosestServerPlayerFrom(GameObject* from) const {
	if (!from) return nullptr;

	const Vector3 origin = from->GetTransform().GetPosition();

	GameObject* best = nullptr;
	float bestD2 = std::numeric_limits<float>::max();

	ForEachServerPlayer([&](GameObject* p) {
		const Vector3 d = p->GetTransform().GetPosition() - origin;
		const float d2 = Vector::Dot(d, d);
		if (d2 < bestD2) {
			bestD2 = d2;
			best = p;
		}
	});
	return best;
}