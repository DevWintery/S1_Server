#pragma once

#include "Creature.h"
#include "Define.h"

class Monster : public Creature
{
public:
	Monster();
	virtual ~Monster();

public:
	virtual void Update() override;
	virtual void TakeDamage(float damage) override;
	
protected:
	virtual void UpdateState();
	virtual void UpdateIdle();
	virtual void UpdateMove();
	virtual void UpdateAttack();

	virtual void ReCalculateDestPos();

public:
	virtual void SetState(Protocol::MonsterState state);
	virtual Protocol::MonsterState GetState() { return _state; }

	void SetMoveMode(EMoveMode moveMode);
	void SetMoveType(EMoveType moveType);
	void SetDestPos(const FVector& destPos) { _destPos = destPos; }
	virtual void SetPos(const FVector& pos) override;

/*Detour Agent*/
public:
	void SetAgentIndex(int idx) { _agentIndex = idx; }
	int GetAgentIndex() { return _agentIndex; }

	FVector GetAgentPos();

private:
	void SetTarget(shared_ptr<Object> object) { _target = object; }

private:
	const uint64 MOVE_TICK = 40;
	const uint64 ATTACK_TICK = 1500;
	const uint64 WAIT_TIME = 3000;
	const float DISTANCE_MAX = 5.0f;
	const float ATTACK_DISTANCE = 1200.f;

	uint64 nextAttackTickAfter = 0;
	uint64 nextMoveTickAfter = 0;
	uint64 nextWaitTickAfter = 0;

private:
	FVector _destPos;
	bool _wait = false;

private:
	EMoveMode _moveMode = EMoveMode::Patrol;
	EMoveType _moveType = EMoveType::Move;
	Protocol::MonsterState _state = Protocol::MONSTER_STATE_IDLE;
	weak_ptr<Object> _target;

	//TEMP
	float _hp = 100.f;

	int _agentIndex = -1;
};

