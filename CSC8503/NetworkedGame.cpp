#include "NetworkedGame.h"
#include "NetworkPlayer.h"
#include "NetworkObject.h"
#include "GameServer.h"
#include "GameClient.h"
#include "GameWorld.h"
#include "Window.h"
#include "Debug.h"
#include "PhysicsObject.h"

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
	float   duration = 1.0f / 20.0f;
};

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
	int seq;
	unsigned char buttons; // bit0=W, bit1=A, bit2=S, bit3=D, bit4=SPACE
	ClientInputPacket() : GamePacket(Message), seq(0), buttons(0) { size = sizeof(int) + sizeof(unsigned char); }
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
	timeToNextPacket  = 0.0f;
	packetsToSnapshot = 0;

	allowLocalPlayerControl = thisServer != nullptr;
}

NetworkedGame::~NetworkedGame()	{
	delete thisServer;
	delete thisClient;
}

void NetworkedGame::StartAsServer() {
	thisServer = new GameServer(NetworkBase::GetDefaultPort(), 4);

	thisServer->RegisterPacketHandler(Received_State, this);
	thisServer->RegisterPacketHandler(Player_Connected, this);
	thisServer->RegisterPacketHandler(Player_Disconnected, this);
	thisServer->RegisterPacketHandler(Message, this); // handle ClientInputPacket

	thisServer->SetGameWorld(world);

	SetLocalPlayerControl(true); // server simulates movement

	// NEW: server status
	std::cout << "[Server] Started on port " << NetworkBase::GetDefaultPort() << " (max clients: 4)" << std::endl;
	//Debug::Print("Server: Running (F10 to connect a client)", Vector2(5, 5), Debug::GREEN);

	StartLevel();
}

void NetworkedGame::StartAsClient(char a, char b, char c, char d) {
	thisClient = new GameClient();
	thisClient->Connect(a, b, c, d, NetworkBase::GetDefaultPort());

	thisClient->RegisterPacketHandler(Delta_State, this);
	thisClient->RegisterPacketHandler(Full_State, this);
	thisClient->RegisterPacketHandler(Player_Connected, this);
	thisClient->RegisterPacketHandler(Player_Disconnected, this);
	thisClient->RegisterPacketHandler(OWNERSHIP_MSG, this); // NEW

	SetLocalPlayerControl(false);
	StartLevel();

	// Keep local player alive; we don't control it client-side
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
		timeToNextPacket += 1.0f / 20.0f; //20hz server/client update
	}

	if (!thisServer && Window::GetKeyboard()->KeyPressed(KeyCodes::F9)) {
		StartAsServer();
	}
	if (!thisClient && Window::GetKeyboard()->KeyPressed(KeyCodes::F10)) {
		StartAsClient(127, 0, 0, 1);
	}

	// Always draw HUD every frame (prevents flicker)
	if (thisServer) {
		Debug::Print("Server: Snapshots sent = " + std::to_string(gBroadcastCount), Vector2(10, 13), Debug::WHITE);
		if (gLastAckFromPeer >= 0) {
			Debug::Print("Last Ack: peer=" + std::to_string(gLastAckFromPeer) +
			             " stateID=" + std::to_string(gLastAckStateId),
			             Vector2(5, 21), Debug::YELLOW);
		}
	}
	if (thisClient) {
		Debug::Print("Client Game", Vector2(0, 95));
	}

	// HUD each frame
	if (thisServer) {
		Debug::Print("Authority: Server", Vector2(0, 88), Debug::GREEN);
	}
	if (thisClient) {
		Debug::Print("Authority: Server (local control disabled)", Vector2(0, 88), Debug::YELLOW);
	}

	TutorialGame::UpdateGame(dt);
}

void NetworkedGame::UpdateAsServer(float dt) {
	thisServer->UpdateServer();   // process connects / inputs / acks
	BroadcastSnapshot(false);     // then broadcast states
	// Remove per-tick prints here; HUD is drawn every frame in UpdateGame
}

void NetworkedGame::UpdateAsClient(float dt) {
	thisClient->UpdateClient();

	// ... existing input / ack send ...

	// Interpolate all proxied players we have smooth targets for
	for (auto& kv : gSmoothPos) {
		const int id = kv.first;
		SmoothPos& s = kv.second;
		s.t += dt;

		float a = s.duration > 0.0f ? std::min(s.t / s.duration, 1.0f) : 1.0f;
		auto it = netIdToObject.find(id);
		if (it != netIdToObject.end() && it->second) {
			GameObject* obj = it->second;
			const Vector3 p = s.from + (s.to - s.from) * a;
			obj->GetTransform().SetPosition(p);
		}
	}
}

void NetworkedGame::BroadcastSnapshot(bool /*deltaFrame*/) {
    std::vector<GameObject*>::const_iterator first;
    std::vector<GameObject*>::const_iterator last;
    world.GetObjectIterators(first, last);

    int packetsThisFrame = 0;

    for (auto i = first; i != last; ++i) {
        GameObject* obj = *i;
        if (!obj) continue;

        const CollisionVolume* vol = obj->GetBoundingVolume();
        if (!vol || vol->isTrigger) continue;

        // Only replicate players
        if (vol->collisionLayer != playerLayer) {
            continue;
        }

        NetworkObject* no = obj->GetNetworkObject();
        if (!no) {
            // Do NOT auto-attach here, StartLevel handles attachment for players
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
	//Periodically remove old data from the server
	int minID = INT_MAX;
	int maxID = 0; //we could use this to see if a player is lagging behind?

	for (auto i : stateIDs) {
		minID = std::min(minID, i.second);
		maxID = std::max(maxID, i.second);
	}
	//every client has acknowledged reaching at least state minID
	//so we can get rid of any old states!
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;
	world.GetObjectIterators(first, last);

	for (auto i = first; i != last; ++i) {
		NetworkObject* o = (*i)->GetNetworkObject();
		if (!o) {
			continue;
		}
		o->UpdateStateHistory(minID); //clear out old states so they arent taking up memory...
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

        const int layer = vol->collisionLayer;
        const bool isPlayer = (layer == playerLayer);

        // Only attach network state for players for now
        if (isPlayer && !obj->GetNetworkObject()) {
            const int id = nextObjectId++;
            obj->SetNetworkObject(new NetworkObject(*obj, id));
            netIdToObject[id] = obj;
        }
    }
}

void NetworkedGame::ReceivePacket(int type, GamePacket* payload, int source) {
	switch (type) {
	case Message: {
		if (thisServer && payload->size == sizeof(int) + sizeof(unsigned char)) {
			auto* input = reinterpret_cast<ClientInputPacket*>(payload);

			GameObject* target = nullptr;
			if (auto it = serverPlayers.find(source); it != serverPlayers.end()) {
				target = it->second;
			}
			// Optional fallback removed; we do not want to use playerObj for peers
			if (!target) {
				std::cout << "[Server] Warning: no player mapped for peer " << source << "\n";
				break;
			}

			if (auto* phys = target->GetPhysicsObject()) {
				Vector3 move(0, 0, 0);
				const float speed = 50.0f * (1.0f / 20.0f);
				if (input->buttons & (1 << 0)) move += Vector3(0, 0, -1); // W
				if (input->buttons & (1 << 2)) move += Vector3(0, 0,  1); // S
				if (input->buttons & (1 << 1)) move += Vector3(-1, 0, 0); // A
				if (input->buttons & (1 << 3)) move += Vector3( 1, 0, 0); // D
				phys->AddForce(move * speed);
				if (input->buttons & (1 << 4)) {
					phys->ApplyLinearImpulse(Vector3(0, 2.0f, 0) * (1.0f / 20.0f));
				}
			}
		}
	} break;

	case Full_State:
		if (thisClient) {
			auto* fp = reinterpret_cast<FullPacket*>(payload);
			GameObject* obj = GetOrCreateProxy(fp->objectID);
			if (!obj) break;

			// Capture pre-snap position
			const Vector3 prevPos = obj->GetTransform().GetPosition();

			if (NetworkObject* no = obj->GetNetworkObject()) {
				no->ReadPacket(*payload); // applies snap
				lastReceivedStateID = std::max(lastReceivedStateID, fp->fullState.stateID);
			}

			// Capture snapped position, then revert and set up smoothing
			const Vector3 snappedPos = obj->GetTransform().GetPosition();
			SmoothPos s{};
			s.from = prevPos;
			s.to = snappedPos;
			s.t = 0.0f;
			s.duration = 1.0f / 20.0f;
			gSmoothPos[fp->objectID] = s;
			obj->GetTransform().SetPosition(prevPos); // start from previous and lerp to snapped

			if (!cameraTarget) {
				SetCameraTarget(obj);
			}

			Debug::Print("Client: Applied Full State ID " + std::to_string(fp->fullState.stateID),
				Vector2(5, 29), Debug::CYAN);
		}
		break;

	case Delta_State:
		if (thisClient) {
			auto* dp = reinterpret_cast<DeltaPacket*>(payload);
			auto it = netIdToObject.find(dp->objectID);
			if (it != netIdToObject.end()) {
				GameObject* obj = it->second;
				const Vector3 prevPos = obj->GetTransform().GetPosition();

				if (NetworkObject* no = obj->GetNetworkObject()) {
					no->ReadPacket(*payload);
					lastReceivedStateID = std::max(lastReceivedStateID, dp->fullID);
				}

				const Vector3 snappedPos = obj->GetTransform().GetPosition();
				SmoothPos s{};
				s.from = prevPos;
				s.to = snappedPos;
				s.t = 0.0f;
				s.duration = 1.0f / 20.0f;
				gSmoothPos[dp->objectID] = s;
				obj->GetTransform().SetPosition(prevPos);
			}
		}
		break;

	case Received_State:
		if (thisServer) {
			auto* ack = reinterpret_cast<AcknowledgePacket*>(payload);
			stateIDs[source] = ack->lastID;

			// NEW: server ack feedback
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

				// NEW: tell this peer which object it owns
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
					world.RemoveGameObject(p, /*andDelete*/true);
				}
			}
			std::cout << "[Server] Player removed for peer " << source << "\n";
		}
		break;

	case OWNERSHIP_MSG:
		if (thisClient) {
			auto* op = reinterpret_cast<OwnershipPacket*>(payload);
			GameObject* mine = GetOrCreateProxy(op->objectID);
			localPlayer = mine;           // use as client ownership marker
			SetCameraTarget(localPlayer); // make camera follow my proxy
			// Optional: HUD hint
			Debug::Print("You own netID " + std::to_string(op->objectID), Vector2(0, 92), Debug::GREEN);
		}
		break;

	default:
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

	// Player proxy; trigger so it won’t collide locally
	playerObject* proxy = AddPlayerToWorld(Vector3(), playerMesh, 3.0f, /*isTrigger*/true, /*collisionLayer*/playerLayer);
	if (!proxy) return nullptr;

	// Make proxy kinematic – don’t let local physics fight server snapshots
	if (auto* phys = proxy->GetPhysicsObject()) {
		phys->SetLinearVelocity(Vector3());
		phys->SetAngularVelocity(Vector3());
		phys->SetInverseMass(0.0f); // static/kinematic on client
	}

	proxy->SetNetworkObject(new NetworkObject(*proxy, objectID));
	netIdToObject[objectID] = proxy;
	return proxy;
}