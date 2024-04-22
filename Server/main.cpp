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

int main(int argc, char* argv[])
{
	srand(time(nullptr));

	ServerPacketHandler::Init();
	std::cout << "ServerPacketHandler Initialize" << std::endl;

	std::size_t len = std::strlen(argv[1]);
	std::wstring wstr(len, L' ');  // ����� ũ���� ���̵� ���ڿ��� �����մϴ�.
	std::mbstowcs(&wstr[0], argv[1], len);  // ��Ƽ����Ʈ�� ���̵� ���ڷ� ��ȯ�մϴ�.

	std::wstring ip = wstr;
	std::string mapName = argv[2];

//#if _DEBUG
//	std::wstring ip = L"192.168.1.3";
//	NavigationSystem::GetInstance()->Init("F:\\S1\\Server\\Hanger.bin");
//#else
//	std::wstring ip = L"127.0.0.1";
//	NavigationSystem::GetInstance()->Init("F:\\S1\\Server\\Hanger.bin");
//#endif

	NavigationSystem::GetInstance()->Init(mapName);


	shared_ptr<ServerService> service = std::make_shared<ServerService>(
		NetAddress(ip, 7777),
		make_shared<IocpCore>(),
		make_shared<GameSession>, // TODO : SessionManager ��
		100);
	std::cout << "Service Created." << std::endl;

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