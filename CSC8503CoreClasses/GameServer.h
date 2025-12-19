#pragma once
#include "NetworkBase.h"
#include <unordered_map>

struct _ENetPeer; // forward declare ENet

namespace NCL {
	namespace CSC8503 {
		class GameWorld;
		class GameServer : public NetworkBase {
		public:
			GameServer(int onPort, int maxClients);
			~GameServer();

			bool Initialise();
			void Shutdown();

			void SetGameWorld(GameWorld& g);

			bool SendGlobalPacket(int msgID);
			bool SendGlobalPacket(GamePacket& packet);

			// NEW: send to a single peer by its incomingPeerID
			bool SendPacketToPeer(int peerId, GamePacket& packet);

			virtual void UpdateServer();

		protected:
			int			port;
			int			clientMax;
			int			clientCount;
			GameWorld* gameWorld;

			int incomingDataRate;
			int outgoingDataRate;

			// NEW: track peers so we can unicast
			std::unordered_map<int, _ENetPeer*> peers;
		};
	}
}