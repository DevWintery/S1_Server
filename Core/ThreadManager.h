#pragma once

/*-----------------
* ThreadManager
*
* Thread 관리 클래스
------------------- */
class ThreadManager
{
public:
	ThreadManager();
	~ThreadManager();

public:
	void Launch(function<void(void)> callback);
	void Join();

	static void InitTLS();
	static void DestroyTLS();

	static void DoGlobalQueueWork();
	static void DistributeReservedJobs();

private:
	mutex			_lock;
	vector<thread>	_threads;
};

