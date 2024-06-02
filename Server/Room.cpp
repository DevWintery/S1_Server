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
			info->CopyFrom(*obj.second->GetInfo());
			//info->set_player_name(obj.second->GetInfo()->player_name());
			
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
	enterGamePkt.set_map_name(_mapName);
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
		object->GetPos()->set_x(StartPos.X + Utils::GetRandom(-150.f, 150.f));
		object->GetPos()->set_y(StartPos.Y + Utils::GetRandom(-150.f, 150.f));
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

	std::vector<ns::Monster> monsterInfos = JsonUtil::ParseJson(_mapName + "_Monster.json", senarioStep);
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

	Protocol::S_LEAVE_GAME pkt;
	shared_ptr<SendBuffer> sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
	
	if (shared_ptr<GameSession> session = player->GetSession().lock())
	{
		cout << "LEAVE PLAYER" << endl;
		session->Send(sendBuffer);
	}

	_playerCount --;

	//플레이어가 모두 떠나면 종료
	if(0 >= _playerCount)
	{
		GRoom->DoTimer(3000, []()
			{
				cout << "Exit" << endl;
				quick_exit(0);
			});
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
	else if(_mapName == "Demonstration")
	{
		DemonstrationInteract(pkt);
	}
	else if (_mapName == "Yumin")
	{
		YuminInteract(pkt);
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
			monster->SetMoveType(EMonsterMoveType::Rush);
		}
		else
		{
			monster->SetMoveType(EMonsterMoveType::Patrol);
		}

		if (info.attack_type == "Punch")
		{
			monster->GetInfo()->set_monster_attack_type(Protocol::MONSTER_ATTACK_TYPE_PUNCH);
			monster->SetAttackType(EMonsterAttackType::Punch);
		}
		else
		{
			monster->GetInfo()->set_monster_attack_type(Protocol::MONSTER_ATTACK_TYPE_RIFLE);
			monster->SetAttackType(EMonsterAttackType::Rifle);
		}

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
	cout << "Broadcast SpawnPackets" << endl;
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
		monster->SetMoveType(EMonsterMoveType::Rush);
	}
	else
	{
		monster->SetMoveType(EMonsterMoveType::Patrol);
	}

	if (info.attack_type == "Punch")
	{
		monster->SetAttackType(EMonsterAttackType::Punch);
	}
	else
	{
		monster->SetAttackType(EMonsterAttackType::Rifle);
	}


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

void Room::RoomClear()
{
	_objects.clear();
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
					//std::vector<ns::Monster> monsters = JsonUtil::ParseJson("Hangar.json", _step);
					std::vector<ns::Monster> monsters = JsonUtil::ParseJson(_mapName + "_Monster.json", _step);

					uint64 time = 1000;
					int i = 0;

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

void Room::DemonstrationInteract(Protocol::C_INTERACT pkt)
{
	if (pkt.interact_type() == Protocol::INTERACT_NEXT_STEP)
	{
		_step += 1;

		Protocol::S_INTERACT interactPkt;

		interactPkt.set_object_id(pkt.object_id());
		interactPkt.set_interact_type(Protocol::INTERACT_NEXT_STEP);
		interactPkt.set_step_id(_step);
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(interactPkt);
		Broadcast(sendBuffer);
	}
}

void Room::YuminInteract(Protocol::C_INTERACT pkt)
{
	if (pkt.interact_type() == Protocol::INTERACT_NEXT_STEP)
	{
		_step += 1;

		std::vector<ns::Monster> monsters = JsonUtil::ParseJson(_mapName + "_Monster.json", _step);

		for (const auto& info : monsters)
		{
			SpawnMonster(info);
		}

		Protocol::S_INTERACT interactPkt;

		interactPkt.set_object_id(pkt.object_id());
		interactPkt.set_interact_type(Protocol::INTERACT_NEXT_STEP);
		interactPkt.set_step_id(_step);
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(interactPkt);
		Broadcast(sendBuffer);
	}
}
