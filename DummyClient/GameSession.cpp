#include "pch.h"
#include "GameSession.h"
#include "ClientPacketHandler.h"

void GameSession::OnConnected()
{
	std::cout << "Connected." << endl;

	Protocol::C_ECHO pkt;
	pkt.set_msg("Hello, World");

	auto sendBuf = ClientPacketHandler::MakeSendBuffer(pkt);
	Send(sendBuf);
}

void GameSession::OnDisconnected()
{
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	shared_ptr<PacketSession> session = GetPacketSessionRef();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	// TODO : packetId 대역 체크
	ClientPacketHandler::HandlePacket(session, buffer, len);
}

void GameSession::OnSend(int32 len)
{

}
