#pragma once

#include "JobQueue.h"
#include "Define.h"

class Object;
class Player;
class Monster;

class Room : public JobQueue
{
public:
	Room();
	virtual ~Room();

public:
	bool EnterRoom(shared_ptr<Object> object);
	bool LeaveRoom(shared_ptr<Object> object);

	bool HandleEnterPlayer(shared_ptr<Player> player);
	bool HandleLeavePlayer(shared_ptr<Player> player);
	void HandleMove(Protocol::C_MOVE pkt);
	void HandleServerMove(Protocol::PosInfo* posInfo);
	void HandleMonsterState(Protocol::S_MONSTER_STATE pkt);

	void HandleAttack(Protocol::C_ATTACK pkt);
	void HandleServerAttack(Protocol::S_ATTACK pkt, shared_ptr<Object> target);

	void HandleHit(Protocol::C_HIT pkt);

	void HandleAnimationState(Protocol::C_ANIMATION_STATE pkt);
	void UpdateRoom();

public:
	const vector<shared_ptr<Object>> GetObjects();

private:
	bool AddObject(shared_ptr<Object> object);
	bool RemoveObject(uint64 objectId);

	void SpawnMonster(FVector spawnPos, FVector targetPos, EMoveMode moveMode = EMoveMode::Patrol);

private:
	bool HitCheck(shared_ptr<Object> targetObject, FVector start, FVector end);

private:
	void Broadcast(shared_ptr<SendBuffer> sendBuffer, uint64 exceptId = 0);

private:
	unordered_map<uint64, shared_ptr<Object>> _objects;

private:
	const uint64 DELTA_TIME = 40;

	//TODO : TEMP
	bool Init = false;
	void MapInitialize();
};

//TODO : RoomManager·Î ºÐ¸®
extern shared_ptr<Room> GRoom;