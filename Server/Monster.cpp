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

void Monster::Update()
{
	//죽었으면 모든 Update 없앰
	if (GetState() == Protocol::MONSTER_STATE_DIE)
	{
		return;
	}

	UpdateState();

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
}

void Monster::TakeDamage(float damage)
{
	_hp -= damage;
}

void Monster::SetMoveMode(EMoveMode moveMode)
{
	_moveMode = moveMode;

	if (_moveMode == EMoveMode::Patrol)
	{
		GetPos()->set_speed(100.f);
	}
	else if(_moveMode == EMoveMode::Rush)
	{
		GetPos()->set_speed(600.f);
	}
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

void Monster::SetState(Protocol::MonsterState state)
{
	if (_state == state)
	{
		return;
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

void Monster::ClearPath()
{
	_pathIndex = 1;
	_paths.clear();
}

void Monster::UpdateState()
{
	//죽음 체크
	if (0 >= _hp)
	{
		SetState(Protocol::MONSTER_STATE_DIE);
		return;
	}

	//target이 존재하면
	if (_target.lock())
	{
		//TODO : _target이 죽거나 그랬으면 뭐 처리해줘야하는데 여기서 안할수도?
		SetState(Protocol::MONSTER_STATE_ATTACK);
		return;
	}

	//타겟이 없으면 타겟을 검색한다.
	for (const auto& object : GRoom->GetObjects())
	{
		if (object->GetInfo()->creature_type() != Protocol::CREATURE_TYPE_PLAYER)
		{
			continue;
		}

		FVector objectPos = object->GetPosVector();
		FVector myPos = GetPosVector();

		if (ATTACK_DISTANCE >= FVector::Distance(objectPos, myPos))
		{
			std::cout << "[MonsterInfo] : Set Target Successed" << std::endl;
			SetTarget(object);
			SetState(Protocol::MONSTER_STATE_ATTACK);
			return;
		}
	}

	//검색을 했는데도 없으면 일상생활로 복귀
	if (_target.lock() == nullptr)
	{
		//이동하다가 쉬고있는데
		if (_wait)
		{
			//이동할떄 됐으면
			if (GetTickCount64() >= nextWaitTickAfter)
			{
				//움직이기
				_wait = false;
				SetState(Protocol::MONSTER_STATE_MOVE);
			}
			else
			{
				SetState(Protocol::MONSTER_STATE_IDLE);
			}

			return;
		}

		if (_moveType == EMoveType::Move)
		{
			SetState(Protocol::MONSTER_STATE_MOVE);
		}
		else
		{
			SetState(Protocol::MONSTER_STATE_IDLE);
		}
		return;
	}
}

void Monster::UpdateIdle()
{	
	//얘는 그냥 쉬는애
	if (_moveType == EMoveType::Fix)
	{
		return;
	}
}

void Monster::UpdateMove()
{
	if (nextMoveTickAfter > GetTickCount64())
	{
		return;
	}
	nextMoveTickAfter = GetTickCount64() + MOVE_TICK;

	//경로 없으면
	if (0 >= _paths.size())
	{
		ReCalculateDestPos(); //위치 정해주기
	}

	FVector pathPos = _paths[_pathIndex];
	pathPos.Z += 90.0f; //Mesh 높이 처리
	FVector delta = pathPos - GetPosVector();
	float distance = delta.Size();
	float deltaTime = MOVE_TICK / 1000.f;

	float ratio = GetPos()->speed() * deltaTime / distance;
	FVector targetPos = GetPosVector() + delta * ratio;

	GetPos()->set_x(targetPos.X);
	GetPos()->set_y(targetPos.Y);
	GetPos()->set_z(targetPos.Z);

	GRoom->DoAsync(&Room::HandleServerMove, GetPos());

	if (GetPos()->speed() * deltaTime > distance)
	{
		_pathIndex++;

		FVector pathLoc;
		if (_pathIndex >= _paths.size())
		{
			pathLoc = _paths[_pathIndex - 1];
		}
		else
		{
			pathLoc = _paths[_pathIndex];
		}
		std::cout << "X : " << pathLoc.X << " Y : " << pathLoc.Y << " Z : " << pathLoc.Z << std::endl;

		if (_pathIndex >= _paths.size())
		{
			ClearPath();

			if (_moveMode == EMoveMode::Patrol)
			{
				std::cout << "[MonsterInfo] : Move complete, Recalculate dest path and wait " << WAIT_TIME / 1000 << "seconds" << std::endl;
				nextWaitTickAfter = GetTickCount64() + WAIT_TIME;
				_wait = true;
			}
			else
			{
				std::cout << "[MonsterInfo] : Move Complete " << std::endl;
				SetState(Protocol::MONSTER_STATE_ATTACK);
			}
		}
		else
		{
			FVector pathLoc = _paths[_pathIndex];
			std::cout << "[MonsterInfo] : Move to next path X : " << pathLoc.X << " Y : " << pathLoc.Y << " Z : " << pathLoc.Z << std::endl;
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

void Monster::ReCalculateDestPos()
{
	if (_moveType == EMoveType::Fix)
	{
		return;
	}

	if (_moveMode == EMoveMode::Patrol)
	{
		_paths = NavigationSystem::GetInstance()->GetRandomPath(GetPosVector());
	}
	else
	{
		_paths = NavigationSystem::GetInstance()->GetPaths(GetPosVector(), _destPos);
	}
}