#pragma once

class Monster;

class ObjectUtils
{
public:
	static int64 GetID();

	static shared_ptr<Player> CreatePlayer(shared_ptr<GameSession> session);
	static shared_ptr<Monster> CreateMonster();

private:
	static atomic<int64> s_idGenerator;
};

