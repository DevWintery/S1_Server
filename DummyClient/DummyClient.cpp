#include "pch.h"
#include "Service.h"
#include "ThreadManager.h"
#include "GameSession.h"
#include "ClientPacketHandler.h"
#include "Protocol.pb.h"

int main()
{
	ClientPacketHandler::Init();

	this_thread::sleep_for(1s);

	shared_ptr<ClientService> service = std::make_shared<ClientService>(
		NetAddress(L"127.0.0.1", 7777),
		make_shared<IocpCore>(),
		make_shared<GameSession>, // TODO : SessionManager µî
		1);

	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([=]()
			{
				while (true)
				{
					service->GetIocpCore()->Dispatch();
				}
			});
	}

	GThreadManager->Join();
}