#include"pch.h"
#include"ServerPacketHandler.h"
#include "ObjectUtils.h"
#include "Room.h"
#include "Player.h"
#include "NavigationSystem.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(SharedPacketSession& session, BYTE* buffer, int32 len)
{
	return true;
}

bool Handle_C_ENTER_ROOM(SharedPacketSession& session, Protocol::C_ENTER_ROOM& pkt)
{
	shared_ptr<Player> player = ObjectUtils::CreatePlayer(static_pointer_cast<GameSession>(session));
	player->GetInfo()->set_player_name(pkt.player_name());

	GRoom->DoAsync(&Room::EnterRoom, player);

	return true;
}

bool Handle_C_ENTER_GAME(SharedPacketSession& session, Protocol::C_ENTER_GAME& pkt)
{
	NavigationSystem::GetInstance()->Init(pkt.map_name() + ".bin");
	
	GRoom->DoAsync(&Room::EnterGame, pkt);

	return true;
}

bool Handle_C_GAME_INIT(SharedPacketSession& session, Protocol::C_GAME_INIT& pkt)
{
	GRoom->DoAsync(&Room::GameInit, pkt);

	return true;
}

bool Handle_C_LEAVE_GAME(SharedPacketSession& session, Protocol::C_LEAVE_GAME& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	shared_ptr<Player> player = gameSession->player.load();
	if (player == nullptr)
		return false;

	GRoom->DoAsync(&Room::HandleLeavePlayer, player);

	return true;
}

bool Handle_C_MOVE(SharedPacketSession& session, Protocol::C_MOVE& pkt)
{
	GRoom->DoAsync(&Room::HandleMove, pkt);

	return true;
}

bool Handle_C_ANIMATION_STATE(SharedPacketSession& session, Protocol::C_ANIMATION_STATE& pkt)
{
	GRoom->DoAsync(&Room::HandleAnimationState, pkt);

	return true;
}

bool Handle_C_ATTACK(SharedPacketSession& session, Protocol::C_ATTACK& pkt)
{
	GRoom->DoAsync(&Room::HandleAttack, pkt);

	return true;
}

bool Handle_C_HIT(SharedPacketSession& session, Protocol::C_HIT& pkt)
{
	GRoom->DoAsync(&Room::HandleHit, pkt);

	return true;
}

bool Handle_C_INTERACT(SharedPacketSession& session, Protocol::C_INTERACT& pkt)
{
	GRoom->DoAsync(&Room::HandleInteract, pkt);

	return true;
}

bool Handle_C_CHANGE_WEAPON(SharedPacketSession& session, Protocol::C_CHANGE_WEAPON& pkt)
{
	GRoom->DoAsync(&Room::HandleChangeWeapon, pkt);

	return true;
}
