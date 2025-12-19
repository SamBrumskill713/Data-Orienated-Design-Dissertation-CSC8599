#pragma once
#include "TutorialGame.h"
#include "NetworkBase.h"
#include <functional>

namespace NCL::CSC8503 {
	class GameServer;
	class GameClient;
	class NetworkPlayer;
	class NetworkObject;
	class EnemyObject;

	class NetworkedGame : public TutorialGame, public PacketReceiver 
	{
	public:
		NetworkedGame(GameWorld& gameWorld, GameTechRendererInterface& renderer, PhysicsSystem& physics);
		~NetworkedGame();

		void StartAsServer();
		void StartAsClient(char a, char b, char c, char d);

		void UpdateGame(float dt) override;

		void SpawnPlayer();

		void StartLevel();

		void ReceivePacket(int type, GamePacket* payload, int source) override;

		void OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b);

		// Iterate server-side players: server host (localPlayer / playerObj) + all clients (serverPlayers)
		void ForEachServerPlayer(const std::function<void(GameObject*)>& fn) const;

		// Convenience for AI: find the closest server-side player to 'from'
		GameObject* FindClosestServerPlayerFrom(GameObject* from) const;

		void OnEnemySpawned(EnemyObject& enemy) override;

		// Optional: host accessor (server-only)
		GameObject* GetHostPlayer() const { return localPlayer; }

		// Role helpers
		bool IsServer() const { return thisServer != nullptr; }
		bool IsClient() const { return thisClient != nullptr; }

	protected:
		void UpdateAsServer(float dt);
		void UpdateAsClient(float dt);

		void BroadcastSnapshot(bool deltaFrame);
		void UpdateMinimumState();

		void RegisterNetworkObject(GameObject* obj, int netId);
		GameObject* GetOrCreateProxy(int objectID);

		std::map<int, int> stateIDs;

		GameServer* thisServer;
		GameClient* thisClient;
		float timeToNextPacket;
		int packetsToSnapshot;

		TutorialGame* mainGame;

		bool allowLocalPlayerControl = false;

		std::vector<NetworkObject*> networkObjects;

		std::map<int, GameObject*> serverPlayers;
		GameObject* localPlayer;

		std::unordered_map<int, GameObject*> netIdToObject;
		int nextObjectId = 1;
		int ownedNetId = -1;

		int lastReceivedStateID = 0;

		// Unique ID ranges for AI so the client can infer object type
		static constexpr int EnemyIdBias = 100000;
		int nextEnemyId = 1;
	};
}