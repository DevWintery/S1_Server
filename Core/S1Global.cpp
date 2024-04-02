#include "pch.h"
#include "S1Global.h"
#include "SocketUtils.h"
#include "ThreadManager.h"
#include "GlobalQueue.h"
#include "JobTimer.h"

ThreadManager* GThreadManager = nullptr;
SendBufferManager* GSendBufferManager;
GlobalQueue* GGlobalQueue = nullptr;
JobTimer* GJobTimer = nullptr;

class S1Global
{
public:
	S1Global()
	{
		GThreadManager = new ThreadManager();
		GSendBufferManager = new SendBufferManager();
		GGlobalQueue = new GlobalQueue();
		GJobTimer = new JobTimer();

		SocketUtils::Init();
	}

	~S1Global()
	{
		delete GThreadManager;
		delete GSendBufferManager;
		delete GGlobalQueue;
		delete GJobTimer;

		SocketUtils::Clear();
	}

} GS1Global;

