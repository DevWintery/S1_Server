#include "pch.h"
#include "Monster.h"
#include "Room.h"
#include "NavigationSystem.h"

Monster::Monster()
{
}

Monster::~Monster()
{

}

void Monster::Initialize()
{
	SetState(Protocol::MONSTER_STATE_IDLE);
}

void Monster::Update()
{
	//죽었으면 모든 Update 없앰
	if (GetState() == Protocol::MONSTER_STATE_DIE)
	{
		return;
	}
	
	if (_target.lock() != nullptr)
	{
		if (_target.lock()->IsDie())
		{
			SetState(Protocol::MONSTER_STATE_IDLE);
			_target.reset();
		}
	}

	shared_ptr<Room> myRoom = room.load().lock();
	for (const auto& player : myRoom->GetPlayers())
	{
		if (player->IsDie())
		{
			continue;
		}

		SetTarget(player);
	}

	SendMovePacket();

	switch (GetState())
	{
		case Protocol::MONSTER_STATE_IDLE:
			UpdateIdle();
		break;
		case Protocol::MONSTER_STATE_MOVE:
			UpdateMove();
		break;
		case Protocol::MONSTER_STATE_ATTACK:
			NavigationSystem::GetInstance()->SetAgentSpeed(GetAgentIndex(), 1.f);
			UpdateAttack();
		break;
	}
}

void Monster::TakeDamage(float damage)
{
	Object::TakeDamage(damage);

	if(_isDie)
	{
		SetState(Protocol::MONSTER_STATE_DIE);
	}
	else
	{
		SetState(Protocol::MONSTER_STATE_ATTACK);
	}
}

void Monster::SetMoveMode(EMoveMode moveMode)
{
	_moveMode = moveMode;
}

void Monster::SetMoveType(EMoveType moveType)
{
	_moveType = moveType;
}

void Monster::SetPos(const FVector& pos)
{
	posInfo->set_x(pos.X);
	posInfo->set_y(pos.Y);
	posInfo->set_z(pos.Z + 90.0f);
}

void Monster::SetTarget(shared_ptr<Object> object)
{
	if (_target.lock() == nullptr)
	{
		_target = object;
		return;
	}

	float origin = FVector::Distance(GetAgentPos(), _target.lock()->GetPosVector());
	float _new = FVector::Distance(GetAgentPos(), object->GetPosVector());

	if (origin > _new)
	{
		_target = object;
	}
}

FVector Monster::GetAgentPos()
{
	const dtCrowdAgent* agent = NavigationSystem::GetInstance()->GetAgent(_agentIndex);
	if (agent == nullptr)
	{
		return FVector::Zero();
	}

	return Utils::RecastToUE5_Meter(FVector(agent->npos[0], agent->npos[1], agent->npos[2]));
}

float Monster::GetAgentSpeed()
{
	const dtCrowdAgent* agent = NavigationSystem::GetInstance()->GetAgent(_agentIndex);
	if (agent == nullptr)
	{
		return 10.f;
	}

	return agent->params.maxSpeed;
}

bool Monster::PerformRaycast(const FVector& startPos, const FVector& endPos)
{
	return NavigationSystem::GetInstance()->Raycast(startPos, endPos);
}


bool Monster::IsTargetInSight(const FVector& targetPosition)
{
	float dist = FVector::Distance(GetAgentPos(), targetPosition);
	if (dist > IN_ATTACK_DISTANCE)
	{
		return false;
	}

	// 여기서 레이캐스트를 수행하여 장애물이 있는지 확인합니다.
	if (PerformRaycast(GetAgentPos(), targetPosition))
	{
		// 레이캐스트가 장애물에 맞았다면, 에이전트는 타겟을 볼 수 없습니다.
		return false;
	}

	// 레이캐스팅을 통해 장애물이 없는지 확인
	return true;
}

void Monster::SetState(Protocol::MonsterState state)
{
	if (_state == state)
	{
		return;
	}

	if (_state == Protocol::MONSTER_STATE_ATTACK)
	{
		_moveMode = EMoveMode::Patrol;
	}

	_state = state;

	Protocol::S_MONSTER_STATE pkt;
	pkt.set_object_id(GetInfo()->object_id());
	pkt.set_state(state);

	int64 targetID = -1;

	shared_ptr<Object> target = _target.lock();
	if (target)
	{
		targetID = target->GetInfo()->object_id();
	}

	pkt.set_target_object_id(targetID);

	std::cout << "[MonsterState] TargetID :  " << targetID << ", State : " << state << std::endl;

	shared_ptr<Room> myRoom = room.load().lock();
	if (myRoom)
	{
		myRoom->DoAsync(&Room::HandleMonsterState, pkt);
	}
}

void Monster::UpdateIdle()
{	
	//얘는 그냥 쉬는애
	if (_moveType == EMoveType::Fix)
	{
		return;
	}

	if (_target.lock())
	{
		float dist = FVector::Distance(GetAgentPos(), _target.lock()->GetPosVector());
		if (ATTACK_DISTANCE > dist)
		{
			SetState(Protocol::MONSTER_STATE_ATTACK);
			return;
		}
	}

	//이동하다가 쉬고있는데
	if (_wait)
	{
		//이동할떄 됐으면
		if (GetTickCount64() >= nextWaitTickAfter)
		{
			//움직이기
			_wait = false;
			_destPos = NavigationSystem::GetInstance()->SetRandomDestination(_agentIndex, SEARCH_RADIUS);
			SetState(Protocol::MONSTER_STATE_MOVE);
		}
	}
	else if(_moveType == EMoveType::Move)
	{
		if (_moveMode == EMoveMode::Rush)
		{
			NavigationSystem::GetInstance()->SetDestination(GetAgentIndex(), _destPos, 8.f);
		}
		else
		{
			_destPos = NavigationSystem::GetInstance()->SetRandomDestination(_agentIndex, SEARCH_RADIUS);
		}
		SetState(Protocol::MONSTER_STATE_MOVE);
	}
}

void Monster::UpdateMove()
{
	if (_target.lock())
	{
		float dist = FVector::Distance(GetAgentPos(), _target.lock()->GetPosVector());
		if (ATTACK_DISTANCE > dist)
		{
			SetState(Protocol::MONSTER_STATE_ATTACK);
			return;
		}
	}

	if (_moveMode == EMoveMode::Patrol)
	{
		FVector recastPos = Utils::UE5ToRecast_Meter(_destPos);
		float targetPos[3] = { recastPos.X, recastPos.Y, recastPos.Z };

		if (NavigationSystem::GetInstance()->IsAgentArrived(GetAgentIndex(), targetPos, 1.f))
		{
			_wait = true;
			nextWaitTickAfter = GetTickCount64() + WAIT_TIME;
			NavigationSystem::GetInstance()->StopAgentMovement(GetAgentIndex());

			SetState(Protocol::MONSTER_STATE_IDLE);
		}
	}
	else
	{
		FVector recastPos = Utils::UE5ToRecast_Meter(_destPos);
		float targetPos[3] = { recastPos.X, recastPos.Y, recastPos.Z };

		if (NavigationSystem::GetInstance()->IsAgentArrived(GetAgentIndex(), targetPos, 1.f))
		{
			NavigationSystem::GetInstance()->StopAgentMovement(GetAgentIndex());

			SetState(Protocol::MONSTER_STATE_ATTACK);
		}
	}
}

void Monster::UpdateAttack()
{
	if (nextAttackTickAfter > GetTickCount64())
	{
		return;
	}
	nextAttackTickAfter = GetTickCount64() + ATTACK_TICK;

	shared_ptr<Object> target = _target.lock();
	if (target == nullptr)
	{
		SetState(Protocol::MONSTER_STATE_IDLE);
		return;
	}

	bool result = IsTargetInSight(target->GetPosVector());
	if (result == false)
	{
		NavigationSystem::GetInstance()->SetDestination(GetAgentIndex(), target->GetPosVector());
	}
	else
	{
		NavigationSystem::GetInstance()->StopAgentMovement(GetAgentIndex());

		Protocol::S_ATTACK attackPkt;
		attackPkt.set_object_id(GetInfo()->object_id());
		attackPkt.set_start_x(GetPosVector().X);
		attackPkt.set_start_y(GetPosVector().Y);
		attackPkt.set_start_z(GetPosVector().Z);

		attackPkt.set_end_x(target->GetPosVector().X + Utils::GetRandom(-100.f, 100.f));
		attackPkt.set_end_y(target->GetPosVector().Y + Utils::GetRandom(-100.f, 100.f));
		attackPkt.set_end_z(target->GetPosVector().Z + Utils::GetRandom(-50.f, 50.f));

		shared_ptr<Room> myRoom = room.load().lock();
		if (myRoom)
		{
			myRoom->DoAsync(&Room::HandleServerAttack, attackPkt, target);
		}
	}
}

void Monster::SendMovePacket()
{
	FVector agentPos = GetAgentPos();

	GetPos()->set_x(agentPos.X);
	GetPos()->set_y(agentPos.Y);
	GetPos()->set_z(agentPos.Z);
	GetPos()->set_speed(GetAgentSpeed());

	shared_ptr<Room> myRoom = room.load().lock();
	if (myRoom)
	{
		myRoom->DoAsync(&Room::HandleServerMove, GetPos());
	}
}
