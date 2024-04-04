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
	ReCalculateDestPos();
	SetState(Protocol::MONSTER_STATE_MOVE);
}

void Monster::Update()
{
	//죽었으면 모든 Update 없앰
	if (GetState() == Protocol::MONSTER_STATE_DIE)
	{
		return;
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
			UpdateAttack();
		break;
	}

	

	//UpdateState();

	//switch (GetState())
	//{
	//	case Protocol::MONSTER_STATE_IDLE:
	//		NavigationSystem::GetInstance()->StopAgentMovement(GetAgentIndex());
	//		UpdateIdle();
	//	break;
	//	case Protocol::MONSTER_STATE_MOVE:
	//		UpdateMove();
	//	break;
	//	case Protocol::MONSTER_STATE_ATTACK:
	//		NavigationSystem::GetInstance()->StopAgentMovement(GetAgentIndex());
	//		UpdateAttack();
	//	break;
	//}
}

void Monster::TakeDamage(float damage)
{
	_hp -= damage;
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

	return Utils::RecastToUE5_Meter(FVector(agent->npos[0], agent->npos[1], agent->npos[2]));
}

float Monster::GetAgentSpeed()
{
	const dtCrowdAgent* agent = NavigationSystem::GetInstance()->GetAgent(_agentIndex);

	return agent->params.maxSpeed;
}

bool Monster::PerformRaycast(const FVector& startPos, const FVector& endPos)
{
	return NavigationSystem::GetInstance()->Raycast(startPos, endPos);
}


bool Monster::IsTargetInSight(const FVector& targetPosition)
{
	float dist = FVector::Distance(GetAgentPos(), targetPosition);
	if (dist > ATTACK_DISTANCE)
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

	std::cout << "[MonsterState] TargetID :  " << "NONE" << ", State : " << state << std::endl;

	_state = state;

	Protocol::S_MONSTER_STATE pkt;
	pkt.set_object_id(GetInfo()->object_id());
	pkt.set_state(state);

	shared_ptr<Object> target = _target.lock();
	if (target)
	{
		pkt.set_target_object_id(target->GetInfo()->object_id());
	}
	else
	{
		pkt.set_target_object_id(-1);
	}


	GRoom->DoAsync(&Room::HandleMonsterState, pkt);
}

void Monster::UpdateState()
{
	//죽음 체크
	if (0 >= _hp)
	{
		SetState(Protocol::MONSTER_STATE_DIE);
		return;
	}

	////target이 존재하면
	//if (_target.lock() != nullptr)
	//{
	//	//TODO : _target이 죽거나 그랬으면 뭐 처리해줘야하는데 여기서 안할수도?
	//	SetState(Protocol::MONSTER_STATE_ATTACK);
	//	return;
	//}

	////타겟이 없으면 타겟을 검색한다.
	//for (const auto& object : GRoom->GetObjects())
	//{
	//	if (object->GetInfo()->creature_type() != Protocol::CREATURE_TYPE_PLAYER)
	//	{
	//		continue;
	//	}

	//	FVector objectPos = object->GetPosVector();
	//	FVector myPos = GetPosVector();

	//	std::cout << FVector::Distance(myPos, objectPos) << std::endl;

	//	if (ATTACK_DISTANCE >= FVector::Distance(myPos, objectPos))
	//	{
	//		std::cout << "[MonsterInfo] : Set Target Successed" << std::endl;
	//		SetTarget(object);
	//		SetState(Protocol::MONSTER_STATE_ATTACK);
	//		return;
	//	}
	//}

	////없으면 뭐 하던대로 둬야지
	////검색을 했는데도 없으면 일상생활로 복귀
	//if (_target.lock() == nullptr)
	//{
	//	return;
	//}
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
			ReCalculateDestPos();
			SetState(Protocol::MONSTER_STATE_MOVE);
		}
	}
	else if(_moveType == EMoveType::Move)
	{
		ReCalculateDestPos();
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
		return;
	}

	bool result = IsTargetInSight(target->GetPosVector());
	if (result == false)
	{
		NavigationSystem::GetInstance()->SetDestination(GetAgentIndex(), target->GetPosVector());

		std::cout << "Tarcking" << std::endl;
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

		GRoom->DoAsync(&Room::HandleServerAttack, attackPkt, target);
	}
}

void Monster::ReCalculateDestPos()
{
	if (_moveType == EMoveType::Fix)
	{
		return;
	}

	if (_moveMode == EMoveMode::Patrol)
	{
		float maxRadius = 10.0f; // 몬스터가 이동할 수 있는 최대 반경
		_destPos = NavigationSystem::GetInstance()->SetRandomDestination(_agentIndex, maxRadius);
	}
	else
	{
		NavigationSystem::GetInstance()->SetDestination(_agentIndex, _destPos, 10.f);
	}
}

void Monster::SendMovePacket()
{
	FVector agentPos = GetAgentPos();

	GetPos()->set_x(agentPos.X);
	GetPos()->set_y(agentPos.Y);
	GetPos()->set_z(agentPos.Z);
	GetPos()->set_speed(GetAgentSpeed());

	GRoom->DoAsync(&Room::HandleServerMove, GetPos());
}
