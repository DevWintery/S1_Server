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
#include "RoomManager.h"

enum
{
	WORKER_TICK = 64
};

void DoWorkerJob(shared_ptr<ServerService>& service)
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + WORKER_TICK;

		// ��Ʈ��ũ ����� ó�� -> �ΰ��� �������� (��Ŷ �ڵ鷯�� ����)
		service->GetIocpCore()->Dispatch(10);

		// ����� �ϰ� ó��
		ThreadManager::DistributeReservedJobs();

		// �۷ι� ť
		ThreadManager::DoGlobalQueueWork();
	}
}

int main()
{
	srand(time(nullptr));

	ServerPacketHandler::Init();

#if _DEBUG
	std::wstring ip = L"192.168.1.3";
	NavigationSystem::GetInstance()->Init("F:\\S1\\Server\\Hanger.bin");
#else
	std::wstring ip = L"127.0.0.1";
	NavigationSystem::GetInstance()->Init("Hanger.bin");
#endif

	shared_ptr<ServerService> service = std::make_shared<ServerService>(
		NetAddress(ip, 7777),
		make_shared<IocpCore>(),
		make_shared<GameSession>, // TODO : SessionManager ��
		100);

	ASSERT_CRASH(service->Start());

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