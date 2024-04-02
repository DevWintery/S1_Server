#include "pch.h"
#include "ObjectUtils.h"
#include "Player.h"
#include "Monster.h"
#include "NavigationSystem.h"

atomic<int64> ObjectUtils::s_idGenerator = 1;

std::shared_ptr<Player> ObjectUtils::CreatePlayer(shared_ptr<GameSession> session)
{
	const int64 newId = s_idGenerator.fetch_add(1);

	shared_ptr<Player> player = make_shared<Player>();
	player->GetInfo()->set_object_id(newId);
	player->GetInfo()->set_object_type(Protocol::OBJECT_TYPE_CREATURE);
	player->GetInfo()->set_creature_type(Protocol::CREATURE_TYPE_PLAYER);
	player->GetPos()->set_object_id(newId);

	player->SetSession(session);
	session->player.store(player);

	return player;
}

std::shared_ptr<Monster> ObjectUtils::CreateMonster()
{
	//흠 테이블 ID를 써야할꺼같긴한데 일단 이거 쓰자
	const int64 newId = s_idGenerator.fetch_add(1);

	shared_ptr<Monster> monster = make_shared<Monster>();
	monster->GetInfo()->set_object_id(newId);
	monster->GetInfo()->set_object_type(Protocol::OBJECT_TYPE_CREATURE);
	monster->GetInfo()->set_creature_type(Protocol::CREATURE_TYPE_MONSTER);
	monster->GetPos()->set_object_id(newId);
	monster->GetPos()->set_speed(600.f);
	
	return monster;
}
