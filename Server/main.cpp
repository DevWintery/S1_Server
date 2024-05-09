#include "pch.h"
#include"Service.h"
#include"SocketUtils.h"
#include"ThreadManager.h"
#include "GameSession.h"
#include"ServerPacketHandler.h"
#include"Protocol.pb.h"
#include "GameSessionManager.h"
#include "Room.h"
#include "NavigationSystem.h"
#include "JsonUtil.h"

enum
{
	WORKER_TICK = 64
};

void DoWorkerJob(shared_ptr<ServerService>& service)
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + WORKER_TICK;

		// 네트워크 입출력 처리 -> 인게임 로직까지 (패킷 핸들러에 의해)
		service->GetIocpCore()->Dispatch(10);

		// 예약된 일감 처리
		ThreadManager::DistributeReservedJobs();

		// 글로벌 큐
		ThreadManager::DoGlobalQueueWork();
	}
}

int main(int argc, char* argv[])
{
	srand(time(nullptr));

	ServerPacketHandler::Init();
	std::cout << "ServerPacketHandler Initialize" << std::endl;

	std::size_t len = std::strlen(argv[1]);
	std::wstring wstr(len, L' ');  // 충분한 크기의 와이드 문자열을 생성합니다.
	std::mbstowcs(&wstr[0], argv[1], len);  // 멀티바이트를 와이드 문자로 변환합니다.
	std::wstring ip = wstr;

	shared_ptr<ServerService> service = std::make_shared<ServerService>(
		NetAddress(ip, 7777),
		make_shared<IocpCore>(),
		make_shared<GameSession>, // TODO : SessionManager 등
		100);
	std::cout << "Service Created." << std::endl;

	ASSERT_CRASH(service->Start());

	GRoom->UpdateRoom();

	for (int32 i = 0; i < 10; i++)
	{
		GThreadManager->Launch([&service]()
			{
				while (true)
				{
					DoWorkerJob(service);
				} 
			});
	}

	DoWorkerJob(service);

	GThreadManager->Join();
}