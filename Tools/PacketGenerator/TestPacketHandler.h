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

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

enum : uint16
{
	PKT_{ {pkt.name} } = { {pkt.id} },
	PKT_{ {pkt.name} } = { {pkt.id} },
	PKT_{ {pkt.name} } = { {pkt.id} },
	PKT_{ {pkt.name} } = { {pkt.id} },
	PKT_{ {pkt.name} } = { {pkt.id} },
	PKT_{ {pkt.name} } = { {pkt.id} },
	PKT_{ {pkt.name} } = { {pkt.id} },
	PKT_{ {pkt.name} } = { {pkt.id} },
	PKT_{ {pkt.name} } = { {pkt.id} },
	PKT_{ {pkt.name} } = { {pkt.id} },
};


// Custom Handlers
bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);
bool Handle_{ {pkt.name} }(PacketSessionRef& session, Protocol::C_ECHO&pkt);
bool Handle_{ {pkt.name} }(PacketSessionRef& session, Protocol::C_LOGIN&pkt);
bool Handle_{ {pkt.name} }(PacketSessionRef& session, Protocol::C_ENTER_GAME&pkt);
bool Handle_{ {pkt.name} }(PacketSessionRef& session, Protocol::C_LEAVE_GAME&pkt);

class { {output} }
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_C_ECHO] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket < Protocol::C_ECHO > (Handle_{ {pkt.name} }, session, buffer, len); };
		GPacketHandler[PKT_C_LOGIN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket < Protocol::C_LOGIN > (Handle_{ {pkt.name} }, session, buffer, len); };
		GPacketHandler[PKT_C_ENTER_GAME] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket < Protocol::C_ENTER_GAME > (Handle_{ {pkt.name} }, session, buffer, len); };
		GPacketHandler[PKT_C_LEAVE_GAME] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket < Protocol::C_LEAVE_GAME > (Handle_{ {pkt.name} }, session, buffer, len); };
	}

	static bool HandlePacket(PacketSessionRef & session, BYTE * buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::S_ECHO&pkt) { return MakeSendBuffer(pkt, PKT_{ {pkt.name} }); }
	static SendBufferRef MakeSendBuffer(Protocol::S_LOGIN&pkt) { return MakeSendBuffer(pkt, PKT_{ {pkt.name} }); }
	static SendBufferRef MakeSendBuffer(Protocol::S_ENTER_GAME&pkt) { return MakeSendBuffer(pkt, PKT_{ {pkt.name} }); }
	static SendBufferRef MakeSendBuffer(Protocol::S_LEAVE_GAME&pkt) { return MakeSendBuffer(pkt, PKT_{ {pkt.name} }); }
	static SendBufferRef MakeSendBuffer(Protocol::S_SPAWN&pkt) { return MakeSendBuffer(pkt, PKT_{ {pkt.name} }); }
	static SendBufferRef MakeSendBuffer(Protocol::S_DESPAWN&pkt) { return MakeSendBuffer(pkt, PKT_{ {pkt.name} }); }

private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, PacketSessionRef & session, BYTE * buffer, int32 len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
			return false;

		return func(session, pkt);
	}

	template<typename T>
	static SendBufferRef MakeSendBuffer(T & pkt, uint16 pktId)
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
		sendBuffer->Close(packetSize);

		return sendBuffer;
	}
};