#include "pch.h"
#include "ClientPacketHandler.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(shared_ptr<PacketSession>& session, BYTE* buffer, int32 len)
{
	return true;
}

bool Handle_S_ECHO(shared_ptr<PacketSession>& session, Protocol::S_ECHO& pkt)
{
	std::cout << "Recv : " << pkt.msg() << endl;

	Protocol::C_ECHO sendPkt;
	sendPkt.set_msg(pkt.msg());

	auto sendBuf = ClientPacketHandler::MakeSendBuffer(sendPkt);
	session->Send(sendBuf);

	return true;
}
