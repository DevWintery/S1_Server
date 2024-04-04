#include "pch.h"
#include "Room.h"
#include "Utils.h"
#include "ServerPacketHandler.h"
#include "Object.h"
#include "Player.h"
#include "ObjectUtils.h"
#include "Monster.h"
#include "NavigationSystem.h"

shared_ptr<Room> GRoom = make_shared<Room>();

Room::Room()
{
}

Room::~Room()
{

}

bool Room::EnterRoom(shared_ptr<Object> object)
{
	//TODO : TEMP
	if (Init == false)
	{
		_step = 0;
		MapInitialize();
	}

	bool success = AddObject(object);

	FVector StartPos = FVector(-10589.000000, -11471.000000, 102.020608);
	object->GetPos()->set_x(StartPos.X + Utils::GetRandom(-250.f, 250.f));
	object->GetPos()->set_y(StartPos.Y + Utils::GetRandom(-250.f, 250.f));
	object->GetPos()->set_z(StartPos.Z);
	object->GetPos()->set_yaw(Utils::GetRandom(0.f, 100.f));

	// 입장 사실을 신입 플레이어에게 알린다
	if (auto player = dynamic_pointer_cast<Player>(object))
	{
		Protocol::S_ENTER_GAME enterGamePkt;
		enterGamePkt.set_success(success);

		Protocol::ObjectInfo* playerInfo = new Protocol::ObjectInfo();
		playerInfo->CopyFrom(*object->GetInfo());
		enterGamePkt.set_allocated_player(playerInfo);

		shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(enterGamePkt);
		if (auto session = player->GetSession().lock())
			session->Send(sendBuffer);
	}

	// 입장 사실을 다른 플레이어에게 알린다
	{
		Protocol::S_SPAWN spawnPkt;

		Protocol::ObjectInfo* objectInfo = spawnPkt.add_objects();
		objectInfo->CopyFrom(*object->GetInfo());

		shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		Broadcast(sendBuffer, object->GetInfo()->object_id());
	}

	// 기존 입장한 플레이어 목록을 신입 플레이어한테 전송해준다
	if (auto player = dynamic_pointer_cast<Player>(object))
	{
		Protocol::S_SPAWN spawnPkt;

		for (auto& item : _objects)
		{
			/*if (item.second->IsPlayer() == false)
				continue;*/

			Protocol::ObjectInfo* objInfo = spawnPkt.add_objects();
			objInfo->CopyFrom(*item.second->GetInfo());
		}

		shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		if (auto session = player->GetSession().lock())
			session->Send(sendBuffer);
	}

	return success;
}

bool Room::LeaveRoom(shared_ptr<Object> object)
{
	if (object == nullptr)
		return false;

	const uint64 objectId = object->GetInfo()->object_id();
	bool success = RemoveObject(objectId);

	// 퇴장 사실을 퇴장하는 플레이어에게 알린다
	if (auto player = dynamic_pointer_cast<Player>(object))
	{
		Protocol::S_LEAVE_GAME leaveGamePkt;

		shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(leaveGamePkt);
		if (auto session = player->GetSession().lock())
			session->Send(sendBuffer);
	}

	// 퇴장 사실을 알린다
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_object_ids(objectId);

		shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(despawnPkt);
		Broadcast(sendBuffer, objectId);

		if (auto player = dynamic_pointer_cast<Player>(object))
			if (auto session = player->GetSession().lock())
				session->Send(sendBuffer);
	}

	return success;
}

bool Room::HandleEnterPlayer(shared_ptr<Player> player)
{
	return EnterRoom(player);
}

bool Room::HandleLeavePlayer(shared_ptr<Player> player)
{
	return LeaveRoom(player);
}

void Room::HandleMove(Protocol::C_MOVE pkt)
{
	const uint64 objectId = pkt.info().object_id();
	if (_objects.find(objectId) == _objects.end())
	{
		return;
	}

	// 적용
	shared_ptr<Player> player = dynamic_pointer_cast<Player>(_objects[objectId]);
	player->GetPos()->CopyFrom(pkt.info());

	// 이동 사실을 알린다 (본인 포함? 빼고?)
	{
		Protocol::S_MOVE movePkt;
		{
			Protocol::PosInfo* info = movePkt.mutable_info();
			info->CopyFrom(pkt.info());
		}
		shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
		Broadcast(sendBuffer);
	}

	//플레이어가 이동했다면 몬스터의 스테이트도 체크한다.
	for (const auto& object : _objects)
	{
		if (object.second->GetInfo()->creature_type() != Protocol::CREATURE_TYPE_MONSTER)
		{
			continue;
		}

		shared_ptr<Monster> monster = dynamic_pointer_cast<Monster>(object.second);
		monster->SetTarget(player);
	}
}

void Room::HandleServerMove(Protocol::PosInfo* posInfo)
{
	const uint64 objectId = posInfo->object_id();
	if (_objects.find(objectId) == _objects.end())
	{
		return;
	}

	//적용
	shared_ptr<Object> object = _objects[objectId];
	object->GetPos()->CopyFrom(*posInfo);

	//보내기
	Protocol::S_MOVE movePkt;
	Protocol::PosInfo* info = movePkt.mutable_info();
	info->CopyFrom(*posInfo);

	shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
	Broadcast(sendBuffer);
}

void Room::HandleMonsterState(Protocol::S_MONSTER_STATE pkt)
{
	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
	Broadcast(sendBuffer);
}

void Room::HandleAttack(Protocol::C_ATTACK pkt)
{
	FVector bulletStartPos = FVector(pkt.start_x(), pkt.start_y(), pkt.start_z());
	FVector bulletEndPos = FVector(pkt.end_x(), pkt.end_y(), pkt.end_z());

	FVector distance = bulletEndPos - bulletStartPos;
	float totalDistance = distance.Size();

	FVector step = distance / totalDistance;

	for (auto object : _objects)
	{
		if (object.second->GetInfo()->creature_type() != Protocol::CREATURE_TYPE_MONSTER)
		{
			continue;
		}

		if (HitCheck(object.second, bulletStartPos, bulletEndPos))
		{
			Protocol::S_HIT hitPkt;

			hitPkt.set_object_id(object.second->GetInfo()->object_id());

			shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(hitPkt);
			Broadcast(sendBuffer);

			std::cout << "[Monster Hit] : ObjectID : " << object.second->GetInfo()->object_id() << std::endl;
			break;
		}
	}

	//다른 애들한테 공격 했다고 알려주기
	Protocol::S_ATTACK attackPkt;

	attackPkt.set_object_id(pkt.object_id());
	attackPkt.set_start_x(pkt.start_x());
	attackPkt.set_start_y(pkt.start_y());
	attackPkt.set_start_z(pkt.start_z());
	attackPkt.set_end_x(pkt.end_x());
	attackPkt.set_end_y(pkt.end_y());
	attackPkt.set_end_z(pkt.end_z());

	shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(attackPkt);
	Broadcast(sendBuffer);
}

void Room::HandleServerAttack(Protocol::S_ATTACK pkt, shared_ptr<Object> target)
{
	{
		shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		Broadcast(sendBuffer);
	}
}

void Room::HandleHit(Protocol::C_HIT pkt)
{
	const uint64 objectId = pkt.object_id();
	if (_objects.find(objectId) == _objects.end())
	{
		return;
	}
	
	_objects[objectId]->TakeDamage(pkt.damage());

	Protocol::S_HIT hitPkt;
	hitPkt.set_object_id(pkt.object_id());
	hitPkt.set_damage(pkt.damage());
	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(hitPkt);
	Broadcast(sendBuffer);
}

void Room::HandleInteract(Protocol::C_INTERACT pkt)
{
	if (pkt.interact_type() == Protocol::INTERACT_NEXT_STEP)
	{
		_step += 1;

		if (_step == 2)
		{
			DoTimer(5000, [this]()
				{
					SpawnMonster(FVector(-1880.f, -12340.f, -18.7f), FVector(-1880.f, -6230.f, -18.7f), EMoveMode::Rush, true);
					SpawnMonster(FVector(-1880.f, -12340.f, -18.7f), FVector(-1880.f, -6230.f, -18.7f), EMoveMode::Rush, true);
					SpawnMonster(FVector(-1880.f, -12340.f, -18.7f), FVector(-1880.f, -6230.f, -18.7f), EMoveMode::Rush, true);
					SpawnMonster(FVector(-1880.f, -12340.f, -18.7f), FVector(-1880.f, -6230.f, -18.7f), EMoveMode::Rush, true);
					SpawnMonster(FVector(-1880.f, -12340.f, -18.7f), FVector(-1880.f, -6230.f, -18.7f), EMoveMode::Rush, true);
					SpawnMonster(FVector(-1880.f, -12340.f, -18.7f), FVector(-1880.f, -6230.f, -18.7f), EMoveMode::Rush, true);
				});
		}

		Protocol::S_INTERACT interactPkt;

		interactPkt.set_object_id(pkt.object_id());
		interactPkt.set_interact_type(Protocol::INTERACT_NEXT_STEP);
		interactPkt.set_step_id(_step);
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(interactPkt);
		Broadcast(sendBuffer);
	}
}

void Room::HandleAnimationState(Protocol::C_ANIMATION_STATE pkt)
{
	Protocol::S_ANIMATION_STATE statePkt;

	statePkt.set_object_id(pkt.object_id());
	statePkt.set_animation_state(pkt.animation_state());

	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(statePkt);
	Broadcast(sendBuffer, pkt.object_id());
}

void Room::UpdateRoom()
{
	NavigationSystem::GetInstance()->Update((float)DELTA_TIME / 1000.f);

	for (const auto& object : _objects)
	{
		object.second->Update();
	}

	//30프레임으로 Room 업데이트
	GRoom->DoTimer(DELTA_TIME, &Room::UpdateRoom);
}

bool Room::AddObject(shared_ptr<Object> object)
{
	// 있다면 문제가 있다.
	if (_objects.find(object->GetInfo()->object_id()) != _objects.end())
		return false;

	_objects.insert(make_pair(object->GetInfo()->object_id(), object));
	
	object->room.store(static_pointer_cast<Room>(shared_from_this()));

	return true;
}

bool Room::RemoveObject(uint64 objectId)
{
	// 없다면 문제가 있다.
	if (_objects.find(objectId) == _objects.end())
		return false;

	shared_ptr<Object> object = _objects[objectId];
	shared_ptr<Player> player = dynamic_pointer_cast<Player>(object);
	if (player)
	{
		player->room.store(weak_ptr<Room>());
	}

	_objects.erase(objectId);

	return true;
}

void Room::SpawnMonster(FVector spawnPos, FVector targetPos, EMoveMode moveMode, bool sendPacket)
{
	shared_ptr<Monster> monster = ObjectUtils::CreateMonster();

	//SpawnPos, TargetPos 설정
	monster->SetMoveType(EMoveType::Move);
	monster->SetMoveMode(moveMode);
	monster->SetPos(spawnPos);
	monster->SetDestPos(targetPos);

	int _agentIndex = NavigationSystem::GetInstance()->AddAgent(Utils::UE5ToRecast_Meter(spawnPos));
	monster->SetAgentIndex(_agentIndex);

	monster->Initialize();

	AddObject(monster);

	if (sendPacket)
	{
		Protocol::S_SPAWN spawnPkt;

		Protocol::ObjectInfo* objectInfo = spawnPkt.add_objects();
		objectInfo->CopyFrom(*monster->GetInfo());

		shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		Broadcast(sendBuffer, monster->GetInfo()->object_id());
	}
}

void Room::MapInitialize()
{
	Init = true;

	SpawnMonster(FVector(-5703.f, -11650.f, -18.7f), FVector(-2580.f, -5360.f, -18.7f));
	//SpawnMonster(FVector(-5703.f, -11650.f, -18.7f), FVector(-2580.f, -5360.f, -18.7f));
	//SpawnMonster(FVector(-5703.f, -11650.f, -18.7f), FVector(-2580.f, -5360.f, -18.7f));
	//SpawnMonster(FVector(-5703.f, -11650.f, -18.7f), FVector(-2580.f, -5360.f, -18.7f));
	//SpawnMonster(FVector(-5703.f, -11650.f, -18.7f), FVector(-2580.f, -5360.f, -18.7f));
	//SpawnMonster(FVector(-5703.f, -11650.f, -18.7f), FVector(-2580.f, -5360.f, -18.7f));
	//SpawnMonster(FVector(-5703.f, -11650.f, -18.7f), FVector(-2580.f, -5360.f, -18.7f));
	//SpawnMonster(FVector(-5703.f, -11650.f, -18.7f), FVector(-2580.f, -5360.f, -18.7f));
	//SpawnMonster(FVector(-5321.f, -11277.f, -18.7f), FVector(-2580.f, -5360.f, -18.7f));

	//SpawnMonster(FVector(-908.f, -10519.f, 1005.7f), FVector(-2580.f, -5360.f, 1005.7f));
	//SpawnMonster(FVector(-908.f, -9270.f, 1005.7f), FVector(-2580.f, -5360.f, 1005.7f));

	//SpawnMonster(FVector(-2159.f, -6507.f, -20.f), FVector(-2580.f, -5360.f, 1005.7f));
	//SpawnMonster(FVector(-1835.f, -6507.f, -20.f), FVector(-2580.f, -5360.f, 1005.7f));
	//SpawnMonster(FVector(-1835.f, -6125.f, -20.f), FVector(-2580.f, -5360.f, 1005.7f));

	//Network Dev
	//SpawnMonster(FVector(1550.f, -210.0f, 0.f), FVector(-970.0f, -210.0f, 0.f), EMoveMode::Rush);
}

bool Room::HitCheck(shared_ptr<Object> targetObject, FVector start, FVector end)
{
	float capsuleHalfHeight = 88.0f; // 캡슐 콜라이더의 높이의 절반
	float capsuleRadius = 34.0f; // 캡슐 콜라이더의 반지름

	FVector ab = FVector(end.X - start.X, end.Y - start.Y, end.Z - start.Z);
	FVector ac = FVector(targetObject->GetPosVector().X - start.X, targetObject->GetPosVector().Y - start.Y, targetObject->GetPosVector().Z - start.Z);
	float abLengthSqr = ab.X * ab.X + ab.Y * ab.Y + ab.Z * ab.Z;
	float abLength = sqrt(abLengthSqr);
	float t = (ac.X * ab.X + ac.Y * ab.Y + ac.Z * ab.Z) / abLengthSqr;

	t = fmax(0, fmin(1, t));

	FVector closestPoint = FVector(start.X + ab.X * t, start.Y + ab.Y * t, start.Z + ab.Z * t );
	float distanceToLine = sqrt(pow(targetObject->GetPosVector().X - closestPoint.X, 2) + pow(targetObject->GetPosVector().Y - closestPoint.Y, 2) + pow(targetObject->GetPosVector().Z - closestPoint.Z, 2));
	float distanceToAxis = sqrt(pow(closestPoint.X - targetObject->GetPosVector().X, 2) + pow(closestPoint.Y - targetObject->GetPosVector().Y, 2) + pow(closestPoint.Z - targetObject->GetPosVector().Z, 2));

	return distanceToLine <= (capsuleRadius * 2) && distanceToAxis <= capsuleHalfHeight;
}

void Room::Broadcast(shared_ptr<SendBuffer> sendBuffer, uint64 exceptId /*= 0*/)
{
	for (auto& item : _objects)
	{
		shared_ptr<Player> player = dynamic_pointer_cast<Player>(item.second);
		if (player == nullptr)
			continue;
		if (player->GetInfo()->object_id() == exceptId)
			continue;

		if (shared_ptr<GameSession> session = player->GetSession().lock())
		{
			session->Send(sendBuffer);
		}
	}
}
