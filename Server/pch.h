#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

#ifdef _DEBUG
#pragma comment(lib, "lib\\S1\\Debug\\Core.lib")
#pragma comment(lib, "lib\\Protobuf\\Debug\\libprotobufd.lib")
#else
#pragma comment(lib, "lib\\S1\\Release\\Core.lib")
#pragma comment(lib, "lib\\Protobuf\\Release\\libprotobuf.lib")
#endif

#include "S1Core.h"
#include "Protocol.pb.h"
#include "GameSession.h"
#include "Utils.h"
#include "Math.h"
#include "ServerPacketHandler.h"

#define SEND_PACKET(pkt)																	\
		shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);		\
		session->Send(sendBuffer);															

