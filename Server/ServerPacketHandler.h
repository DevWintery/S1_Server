#pragma once
#include "Protocol.pb.h"

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1

#include "SendBuffer.h"

using SharedPacketSession = TSharedPtr<class PacketSession>;
using SharedSendBuffer = TSharedPtr<class SendBuffer>;

#else

using SharedPacketSession = shared_ptr<class PacketSession>;
using SharedSendBuffer = shared_ptr<class SendBuffer>;

#endif

using PacketHandlerFunc = std::function<bool(SharedPacketSession&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

enum : uint16
{
	PKT_C_ENTER_ROOM = 1000,
	PKT_S_ENTER_ROOM = 1001,
	PKT_C_ENTER_GAME = 1002,
	PKT_S_ENTER_GAME = 1003,
	PKT_C_GAME_INIT = 1004,
	PKT_S_GAME_INIT = 1005,
	PKT_C_LEAVE_GAME = 1006,
	PKT_S_LEAVE_GAME = 1007,
	PKT_S_SPAWN = 1008,
	PKT_S_DESPAWN = 1009,
	PKT_C_MOVE = 1010,
	PKT_S_MOVE = 1011,
	PKT_S_MONSTER_STATE = 1012,
	PKT_C_ANIMATION_STATE = 1013,
	PKT_S_ANIMATION_STATE = 1014,
	PKT_C_ATTACK = 1015,
	PKT_S_ATTACK = 1016,
	PKT_C_HIT = 1017,
	PKT_S_HIT = 1018,
	PKT_C_INTERACT = 1019,
	PKT_S_INTERACT = 1020,
	PKT_C_CHANGE_WEAPON = 1021,
	PKT_S_CHANGE_WEAPON = 1022,
};


// Custom Handlers
bool Handle_INVALID(SharedPacketSession& session, BYTE* buffer, int32 len);
bool Handle_C_ENTER_ROOM(SharedPacketSession& session, Protocol::C_ENTER_ROOM&pkt);
bool Handle_C_ENTER_GAME(SharedPacketSession& session, Protocol::C_ENTER_GAME&pkt);
bool Handle_C_GAME_INIT(SharedPacketSession& session, Protocol::C_GAME_INIT&pkt);
bool Handle_C_LEAVE_GAME(SharedPacketSession& session, Protocol::C_LEAVE_GAME&pkt);
bool Handle_C_MOVE(SharedPacketSession& session, Protocol::C_MOVE&pkt);
bool Handle_C_ANIMATION_STATE(SharedPacketSession& session, Protocol::C_ANIMATION_STATE&pkt);
bool Handle_C_ATTACK(SharedPacketSession& session, Protocol::C_ATTACK&pkt);
bool Handle_C_HIT(SharedPacketSession& session, Protocol::C_HIT&pkt);
bool Handle_C_INTERACT(SharedPacketSession& session, Protocol::C_INTERACT&pkt);
bool Handle_C_CHANGE_WEAPON(SharedPacketSession& session, Protocol::C_CHANGE_WEAPON&pkt);

class ServerPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_C_ENTER_ROOM] = [](SharedPacketSession& session, BYTE* buffer, int32 len) { return HandlePacket < Protocol::C_ENTER_ROOM > (Handle_C_ENTER_ROOM, session, buffer, len); };
		GPacketHandler[PKT_C_ENTER_GAME] = [](SharedPacketSession& session, BYTE* buffer, int32 len) { return HandlePacket < Protocol::C_ENTER_GAME > (Handle_C_ENTER_GAME, session, buffer, len); };
		GPacketHandler[PKT_C_GAME_INIT] = [](SharedPacketSession& session, BYTE* buffer, int32 len) { return HandlePacket < Protocol::C_GAME_INIT > (Handle_C_GAME_INIT, session, buffer, len); };
		GPacketHandler[PKT_C_LEAVE_GAME] = [](SharedPacketSession& session, BYTE* buffer, int32 len) { return HandlePacket < Protocol::C_LEAVE_GAME > (Handle_C_LEAVE_GAME, session, buffer, len); };
		GPacketHandler[PKT_C_MOVE] = [](SharedPacketSession& session, BYTE* buffer, int32 len) { return HandlePacket < Protocol::C_MOVE > (Handle_C_MOVE, session, buffer, len); };
		GPacketHandler[PKT_C_ANIMATION_STATE] = [](SharedPacketSession& session, BYTE* buffer, int32 len) { return HandlePacket < Protocol::C_ANIMATION_STATE > (Handle_C_ANIMATION_STATE, session, buffer, len); };
		GPacketHandler[PKT_C_ATTACK] = [](SharedPacketSession& session, BYTE* buffer, int32 len) { return HandlePacket < Protocol::C_ATTACK > (Handle_C_ATTACK, session, buffer, len); };
		GPacketHandler[PKT_C_HIT] = [](SharedPacketSession& session, BYTE* buffer, int32 len) { return HandlePacket < Protocol::C_HIT > (Handle_C_HIT, session, buffer, len); };
		GPacketHandler[PKT_C_INTERACT] = [](SharedPacketSession& session, BYTE* buffer, int32 len) { return HandlePacket < Protocol::C_INTERACT > (Handle_C_INTERACT, session, buffer, len); };
		GPacketHandler[PKT_C_CHANGE_WEAPON] = [](SharedPacketSession& session, BYTE* buffer, int32 len) { return HandlePacket < Protocol::C_CHANGE_WEAPON > (Handle_C_CHANGE_WEAPON, session, buffer, len); };
	}

	static bool HandlePacket(SharedPacketSession& session, BYTE * buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}
	static SharedSendBuffer MakeSendBuffer(Protocol::S_ENTER_ROOM&pkt) { return MakeSendBuffer(pkt, PKT_S_ENTER_ROOM); }
	static SharedSendBuffer MakeSendBuffer(Protocol::S_ENTER_GAME&pkt) { return MakeSendBuffer(pkt, PKT_S_ENTER_GAME); }
	static SharedSendBuffer MakeSendBuffer(Protocol::S_GAME_INIT&pkt) { return MakeSendBuffer(pkt, PKT_S_GAME_INIT); }
	static SharedSendBuffer MakeSendBuffer(Protocol::S_LEAVE_GAME&pkt) { return MakeSendBuffer(pkt, PKT_S_LEAVE_GAME); }
	static SharedSendBuffer MakeSendBuffer(Protocol::S_SPAWN&pkt) { return MakeSendBuffer(pkt, PKT_S_SPAWN); }
	static SharedSendBuffer MakeSendBuffer(Protocol::S_DESPAWN&pkt) { return MakeSendBuffer(pkt, PKT_S_DESPAWN); }
	static SharedSendBuffer MakeSendBuffer(Protocol::S_MOVE&pkt) { return MakeSendBuffer(pkt, PKT_S_MOVE); }
	static SharedSendBuffer MakeSendBuffer(Protocol::S_MONSTER_STATE&pkt) { return MakeSendBuffer(pkt, PKT_S_MONSTER_STATE); }
	static SharedSendBuffer MakeSendBuffer(Protocol::S_ANIMATION_STATE&pkt) { return MakeSendBuffer(pkt, PKT_S_ANIMATION_STATE); }
	static SharedSendBuffer MakeSendBuffer(Protocol::S_ATTACK&pkt) { return MakeSendBuffer(pkt, PKT_S_ATTACK); }
	static SharedSendBuffer MakeSendBuffer(Protocol::S_HIT&pkt) { return MakeSendBuffer(pkt, PKT_S_HIT); }
	static SharedSendBuffer MakeSendBuffer(Protocol::S_INTERACT&pkt) { return MakeSendBuffer(pkt, PKT_S_INTERACT); }
	static SharedSendBuffer MakeSendBuffer(Protocol::S_CHANGE_WEAPON&pkt) { return MakeSendBuffer(pkt, PKT_S_CHANGE_WEAPON); }

private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, SharedPacketSession& session, BYTE * buffer, int32 len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
			return false;

		return func(session, pkt);
	}

	template<typename T>
	static SharedSendBuffer MakeSendBuffer(T & pkt, uint16 pktId)
	{
		const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1

		TSharedPtr<SendBuffer> sendBuffer = MakeShared<SendBuffer>(packetSize);
#else
		shared_ptr<SendBuffer> sendBuffer = GSendBufferManager->Open(packetSize);
#endif
		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		header->size = packetSize;
		header->id = pktId;
		pkt.SerializeToArray(&header[1], dataSize);
		sendBuffer->Close(packetSize);

		return sendBuffer;
	}
};