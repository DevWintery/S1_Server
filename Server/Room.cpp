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

std::vector<std::shared_ptr<Object>> Room::GetPlayers()
{
	std::vector<shared_ptr<Object>> players;

	for (const auto& object : _objects)
	{
		if (object.second->GetInfo()->creature_type() == Protocol::CREATURE_TYPE_PLAYER)
		{
			players.push_back(object.second);
		}
	}

	return players;
}

bool Room::EnterRoom(shared_ptr<Player> player)
{
	AddObject(player);

	Protocol::S_ENTER_ROOM pkt;

	for(const auto& obj : _objects)
	{
		if(obj.second->GetInfo()->creature_type() == Protocol::CREATURE_TYPE_PLAYER)
		{
			Protocol::ObjectInfo* info = pkt.add_players();
			info->set_player_name(obj.second->GetInfo()->player_name());

			std::cout << obj.second->GetInfo()->player_name() << std::endl;
		}
	}

	shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
	
	Broadcast(sendBuffer);

	return true;
}

bool Room::EnterGame(Protocol::C_ENTER_GAME pkt)
{
	senarioStep = 0;
	_mapName = pkt.map_name();

	Protocol::S_ENTER_GAME enterGamePkt;
	shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(enterGamePkt);

	Broadcast(sendBuffer);

	return true;
}

bool Room::GameInit(Protocol::C_GAME_INIT pkt)
{
	if (_initialize)
	{
		return true;
	}

	FVector StartPos = JsonUtil::ParsePlayerJson(_mapName + "_Player.json");

	for (const auto& obj : _objects)
	{
		shared_ptr<Object> object = obj.second;

		//시작 위치 세팅
		object->GetPos()->set_x(StartPos.X + Utils::GetRandom(-250.f, 250.f));
		object->GetPos()->set_y(StartPos.Y + Utils::GetRandom(-250.f, 250.f));
		object->GetPos()->set_z(StartPos.Z);
		object->GetPos()->set_yaw(Utils::GetRandom(0.f, 100.f));

		//플레이어 생성
		Protocol::S_GAME_INIT initPkt;
		if (auto player = dynamic_pointer_cast<Player>(object))
		{
			Protocol::ObjectInfo* playerInfo = new Protocol::ObjectInfo();
			playerInfo->CopyFrom(*object->GetInfo());
			initPkt.set_allocated_player(playerInfo);

			shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(initPkt);
			if(auto session = player->GetSession().lock())
				session->Send(sendBuffer);
		}

		{
			Protocol::S_SPAWN spawnPkt;

			Protocol::ObjectInfo* objInfo = spawnPkt.add_objects();
			objInfo->CopyFrom(*object->GetInfo());

			shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
			Broadcast(sendBuffer, object->GetInfo()->object_id());
		}
	}

#if _DEBUG
	std::vector<ns::Monster> monsters = JsonUtil::ParseJson("F:\\S1\\Server\\Hangar.json", 0);
#else
	std::vector<ns::Monster> monsterInfos = JsonUtil::ParseJson(_mapName + "_Monster.json", senarioStep);
#endif

	SpawnMonster(monsterInfos);

	_initialize = true;

	return true;
}

bool Room::HandleLeavePlayer(shared_ptr<Player> player)
{
	const uint64 objectId = player->GetInfo()->object_id();
	if (_objects.find(objectId) == _objects.end())
	{
		return false;
	}

	cout << "LEAVE PLAYER" << endl;

	_objects.erase(objectId);
	_playerCount--;

	//플레이어가 모두 떠나면 종료
	if(0 >= _playerCount)
	{
		cout << "Exit" << endl;
		quick_exit(0);
	}

	return true;
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
	//FVector bulletStartPos = FVector(pkt.start_x(), pkt.start_y(), pkt.start_z());
	//FVector bulletEndPos = FVector(pkt.end_x(), pkt.end_y(), pkt.end_z());

	//FVector distance = bulletEndPos - bulletStartPos;
	//float totalDistance = distance.Size();

	//FVector step = distance / totalDistance;

	//for (auto object : _objects)
	//{
	//	if (object.second->GetInfo()->creature_type() != Protocol::CREATURE_TYPE_MONSTER)
	//	{
	//		continue;
	//	}

	//	if (HitCheck(object.second, bulletStartPos, bulletEndPos))
	//	{
	//		Protocol::S_HIT hitPkt;

	//		hitPkt.set_object_id(object.second->GetInfo()->object_id());

	//		shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(hitPkt);
	//		Broadcast(sendBuffer);

	//		std::cout << "[Monster Hit] : ObjectID : " << object.second->GetInfo()->object_id() << std::endl;
	//		break;
	//	}
	//}

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
	if (_mapName == "Hangar")
	{
		HangarInteract(pkt);
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

void Room::HandleChangeWeapon(Protocol::C_CHANGE_WEAPON pkt)
{
	Protocol::S_CHANGE_WEAPON changePkt;

	int objectId = pkt.object_id();
	int weaponId = pkt.weapon_id();

	changePkt.set_object_id(objectId);
	changePkt.set_weapon_id(weaponId);
	
	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(changePkt);
	Broadcast(sendBuffer, objectId);
}

void Room::UpdateRoom()
{
	NavigationSystem::GetInstance()->Update((float)DELTA_TIME / 1000.f);

	for (const auto& object : _objects)
	{
		object.second->Update();
	}

	DoTimer(DELTA_TIME, &Room::UpdateRoom);
}

bool Room::AddObject(shared_ptr<Object> object)
{
	// 있다면 문제가 있다.
	if (_objects.find(object->GetInfo()->object_id()) != _objects.end())
		return false;

	_objects.insert(make_pair(object->GetInfo()->object_id(), object));
	
	object->room.store(static_pointer_cast<Room>(shared_from_this()));

	if(dynamic_pointer_cast<Player>(object))
	{
		_playerCount++;
	}

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

void Room::SpawnMonster(const std::vector<ns::Monster>& infos)
{
	Protocol::S_SPAWN spawnPkt;
	for (const auto& info : infos)
	{
		shared_ptr<Monster> monster = ObjectUtils::CreateMonster();

		FVector spawnPos = FVector(info.location.x, info.location.y, info.location.z);
		FVector destPos = FVector::Zero();

		if (info.type == "Rush")
		{
			destPos = FVector(info.dest_location->x, info.dest_location->y, info.dest_location->z);
			monster->SetMoveMode(EMoveMode::Rush);
		}
		else
		{
			monster->SetMoveMode(EMoveMode::Patrol);
		}

		//TODO : type 추가
		monster->SetMoveType(EMoveType::Move);
		monster->SetPos(spawnPos);
		monster->SetDestPos(destPos);

		int _agentIndex = NavigationSystem::GetInstance()->AddAgent(Utils::UE5ToRecast_Meter(spawnPos));
		monster->SetAgentIndex(_agentIndex);
		monster->Initialize();
		AddObject(monster);

		Protocol::ObjectInfo* objectInfo = spawnPkt.add_objects();
		objectInfo->CopyFrom(*monster->GetInfo());
	}

	shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
	Broadcast(sendBuffer);
}

void Room::SpawnMonster(const ns::Monster& info)
{
	Protocol::S_SPAWN spawnPkt;

	shared_ptr<Monster> monster = ObjectUtils::CreateMonster();

	FVector spawnPos = FVector(info.location.x, info.location.y, info.location.z);
	FVector destPos = FVector::Zero();

	if (info.type == "Rush")
	{
		destPos = FVector(info.dest_location->x, info.dest_location->y, info.dest_location->z);
		monster->SetMoveMode(EMoveMode::Rush);
	}
	else
	{
		monster->SetMoveMode(EMoveMode::Patrol);
	}

	//TODO : type 추가
	monster->SetMoveType(EMoveType::Move);
	monster->SetPos(spawnPos);
	monster->SetDestPos(destPos);

	int _agentIndex = NavigationSystem::GetInstance()->AddAgent(Utils::UE5ToRecast_Meter(spawnPos));
	monster->SetAgentIndex(_agentIndex);
	monster->Initialize();
	AddObject(monster);

	Protocol::ObjectInfo* objectInfo = spawnPkt.add_objects();
	objectInfo->CopyFrom(*monster->GetInfo());

	shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
	Broadcast(sendBuffer);
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

void Room::HangarInteract(Protocol::C_INTERACT pkt)
{
	if (pkt.interact_type() == Protocol::INTERACT_NEXT_STEP)
	{
		_step += 1;

		if (_step == 2)
		{
			DoTimer(5000, [this]()
				{
#if _DEBUG
					std::vector<ns::Monster> monsters = JsonUtil::ParseJson("F:\\S1\\Server\\Hangar.json", _step);
#else
					std::vector<ns::Monster> monsters = JsonUtil::ParseJson("Hangar.json", _step);
#endif
					uint64 time = 1000;
					int i = 0;

					Protocol::S_SPAWN spawnPkt;

					for (const auto& info : monsters)
					{
						DoTimer(time * i, [this, info]()
							{
								SpawnMonster(info);
							});

						i++;
					}
				});

			DoTimer(36000, [this, pkt]()
				{
					_step += 1;

					std::cout << "Send Stage Clear Packet" << std::endl;

					Protocol::S_INTERACT interactPkt;

					interactPkt.set_object_id(pkt.object_id());
					interactPkt.set_interact_type(Protocol::INTERACT_NEXT_STEP);
					interactPkt.set_step_id(_step);
					auto sendBuffer = ServerPacketHandler::MakeSendBuffer(interactPkt);
					Broadcast(sendBuffer);
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
