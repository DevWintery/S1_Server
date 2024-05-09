#pragma once

#include "JobQueue.h"
#include "Define.h"
#include "JsonUtil.h"

class Object;
class Player;
class Monster;

class Room : public JobQueue
{
public:
	Room();
	virtual ~Room();

public:
	std::vector<shared_ptr<Object>> GetPlayers();

public:
	bool EnterRoom(shared_ptr<Player> player);

public:
	bool EnterGame(Protocol::C_ENTER_GAME pkt);
	bool GameInit(Protocol::C_GAME_INIT pkt);

	bool HandleLeavePlayer(shared_ptr<Player> player);
	void HandleMove(Protocol::C_MOVE pkt);
	void HandleServerMove(Protocol::PosInfo* posInfo);
	void HandleMonsterState(Protocol::S_MONSTER_STATE pkt);
	void HandleAttack(Protocol::C_ATTACK pkt);
	void HandleServerAttack(Protocol::S_ATTACK pkt, shared_ptr<Object> target);
	void HandleHit(Protocol::C_HIT pkt);
	void HandleInteract(Protocol::C_INTERACT pkt);
	void HandleAnimationState(Protocol::C_ANIMATION_STATE pkt);
	void HandleChangeWeapon(Protocol::C_CHANGE_WEAPON pkt);

	void UpdateRoom();

private:
	bool AddObject(shared_ptr<Object> object);
	bool RemoveObject(uint64 objectId);

	void SpawnMonster(const std::vector<ns::Monster>& infos);
	void SpawnMonster(const ns::Monster& info);

private:
	bool HitCheck(shared_ptr<Object> targetObject, FVector start, FVector end);

private:
	void Broadcast(shared_ptr<SendBuffer> sendBuffer, uint64 exceptId = 0);

private:
	void HangarInteract(Protocol::C_INTERACT pkt);

private:
	unordered_map<uint64, shared_ptr<Object>> _objects;

private:
	const uint64 DELTA_TIME = 100;

	bool _initialize = false;
	int _step = 0;

private:
	std::string _name;
	std::string _mapName;

	int senarioStep = 0;
	int _playerCount = 0;
};

extern shared_ptr<Room> GRoom;