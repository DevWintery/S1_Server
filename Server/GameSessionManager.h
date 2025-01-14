#pragma once

class GameSession;

class GameSessionManager
{
public:
	void Add(shared_ptr<GameSession> session);
	void Remove(shared_ptr<GameSession> session);
	void Broadcast(shared_ptr<SendBuffer> sendBuffer);

private:
	USE_LOCK;
	set<shared_ptr<GameSession>> _sessions;
};

extern GameSessionManager GSessionManager;
