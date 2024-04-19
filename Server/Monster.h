#pragma once

#include "Creature.h"
#include "Define.h"

class Monster : public Creature
{
public:
	Monster();
	virtual ~Monster();

public:
	virtual void Initialize() override;
	virtual void Update() override;
	virtual void TakeDamage(float damage) override;
	
protected:
	virtual void UpdateIdle();
	virtual void UpdateMove();
	virtual void UpdateAttack();
private:
	void SendMovePacket();

public:
	virtual void SetState(Protocol::MonsterState state);
	virtual Protocol::MonsterState GetState() { return _state; }

	void SetMoveMode(EMoveMode moveMode);
	void SetMoveType(EMoveType moveType);
	void SetDestPos(const FVector& destPos) { _destPos = destPos; }
	virtual void SetPos(const FVector& pos) override;
	void SetTarget(shared_ptr<Object> object);

/*Detour Agent*/
public:
	void SetAgentIndex(int idx) { _agentIndex = idx; }
	int GetAgentIndex() { return _agentIndex; }

	FVector GetAgentPos();
	float GetAgentSpeed();

	bool PerformRaycast(const FVector& startPos, const FVector& endPos);
	bool IsTargetInSight(const FVector& targetPosition);

private:
	const uint64 ATTACK_TICK = 1500;
	const uint64 WAIT_TIME = 5000;
	const float ATTACK_DISTANCE = 2500.f;
	const float IN_ATTACK_DISTANCE = 4000.f;
	const float SEARCH_RADIUS = 3.f;

	uint64 nextAttackTickAfter = 0;
	uint64 nextWaitTickAfter = 0;

private:
	FVector _destPos;
	bool _wait = false;

private:
	EMoveMode _moveMode = EMoveMode::Patrol;
	EMoveType _moveType = EMoveType::Move;
	Protocol::MonsterState _state = Protocol::MONSTER_STATE_IDLE;
	weak_ptr<Object> _target;



	int _agentIndex = -1;
};

