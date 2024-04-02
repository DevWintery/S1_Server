#pragma once

#define WIN32_LEAN_AND_MEAN // ���� ������ �ʴ� ������ Windows ������� �����մϴ�.

#ifdef _DEBUG
#pragma comment(lib, "lib\\S1\\Debug\\Core.lib")
#pragma comment(lib, "lib\\Protobuf\\Debug\\libprotobufd.lib")
#else
#pragma comment(lib, "lib\\S1\\Release\\Core.lib")
#pragma comment(lib, "lib\\Protobuf\\Release\\libprotobuf.lib")
#endif

#include "S1Core.h"

#define SEND_PACKET(pkt)																	\
		shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);		\
		session->Send(sendBuffer);												