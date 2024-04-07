#include"pch.h"
#include"ServerPacketHandler.h"
#include "ObjectUtils.h"
#include "Room.h"
#include "Player.h"
#include "RoomManager.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(SharedPacketSession& session, BYTE* buffer, int32 len)
{
	return true;
}

bool Handle_C_CREATE_ROOM(SharedPacketSession& session, Protocol::C_CREATE_ROOM& pkt)
{
	shared_ptr<Player> player = ObjectUtils::CreatePlayer(static_pointer_cast<GameSession>(session));
	player->GetInfo()->set_player_name(pkt.player_name());

	std::string roomName = pkt.room_info().name();
	std::string mapName = pkt.room_info().map();

	RoomManager::GetInstance()->CreateRoom(player, roomName, mapName);

	return true;
}

bool Handle_C_ROOM_LIST(SharedPacketSession& session, Protocol::C_ROOM_LIST& pkt)
{
	Protocol::S_ROOM_LIST roomPkt;

	for (const auto& room : RoomManager::GetInstance()->RoomList())
	{
		Protocol::RoomInfo* info = roomPkt.add_room_infos();
		info->set_room_id(room.first);
		info->set_name(room.second->GetRoomName());
		info->set_map(room.second->GetRoomMapName());
	}

	SEND_PACKET(roomPkt);

	return true;
}

bool Handle_C_ENTER_ROOM(SharedPacketSession& session, Protocol::C_ENTER_ROOM& pkt)
{
	shared_ptr<Player> player = ObjectUtils::CreatePlayer(static_pointer_cast<GameSession>(session));

	player->GetInfo()->set_player_name(pkt.player_name());

	RoomManager::GetInstance()->EnterRoom(player, pkt);

	return true;
}

bool Handle_C_LOGIN(SharedPacketSession& session, Protocol::C_LOGIN& pkt)
{
	//TODO : DB

	Protocol::S_LOGIN loginPkt;

	//TODO : 일단은 무조건 성공 Validation 필요
	loginPkt.set_success(true);

	SEND_PACKET(loginPkt);

	return true;
}

bool Handle_C_ENTER_GAME(SharedPacketSession& session, Protocol::C_ENTER_GAME& pkt)
{
	//shared_ptr<Player> player = ObjectUtils::CreatePlayer(static_pointer_cast<GameSession>(session));

	// 방에 입장

	RoomManager::GetInstance()->EnterGame(pkt.room_id());

	//GRoom->DoAsync(&Room::RoomSetting);
	//GRoom->DoAsync(&Room::HandleEnterPlayer, player);

	return true;
}

bool Handle_C_LEAVE_GAME(SharedPacketSession& session, Protocol::C_LEAVE_GAME& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	shared_ptr<Player> player = gameSession->player.load();
	if (player == nullptr)
		return false;

	shared_ptr<Room> room = player->room.load().lock();
	if (room == nullptr)
		return false;

	room->DoAsync(&Room::HandleLeavePlayer, player);

	return true;
}

bool Handle_C_MOVE(SharedPacketSession& session, Protocol::C_MOVE& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	shared_ptr<Player> player = gameSession->player.load();
	if (player == nullptr)
		return false;

	shared_ptr<Room> room = player->room.load().lock();
	if (room == nullptr)
		return false;

	room->DoAsync(&Room::HandleMove, pkt);

	return true;
}

bool Handle_C_ANIMATION_STATE(SharedPacketSession& session, Protocol::C_ANIMATION_STATE& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	shared_ptr<Player> player = gameSession->player.load();
	if (player == nullptr)
		return false;

	shared_ptr<Room> room = player->room.load().lock();
	if (room == nullptr)
		return false;

	room->DoAsync(&Room::HandleAnimationState, pkt);

	return true;
}

bool Handle_C_ATTACK(SharedPacketSession& session, Protocol::C_ATTACK& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	shared_ptr<Player> player = gameSession->player.load();
	if (player == nullptr)
		return false;

	shared_ptr<Room> room = player->room.load().lock();
	if (room == nullptr)
		return false;

	room->DoAsync(&Room::HandleAttack, pkt);

	return true;
}

bool Handle_C_HIT(SharedPacketSession& session, Protocol::C_HIT& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	shared_ptr<Player> player = gameSession->player.load();
	if (player == nullptr)
		return false;

	shared_ptr<Room> room = player->room.load().lock();
	if (room == nullptr)
		return false;

	room->DoAsync(&Room::HandleHit, pkt);

	return true;
}

bool Handle_C_INTERACT(SharedPacketSession& session, Protocol::C_INTERACT& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	shared_ptr<Player> player = gameSession->player.load();
	if (player == nullptr)
		return false;

	shared_ptr<Room> room = player->room.load().lock();
	if (room == nullptr)
		return false;

	room->DoAsync(&Room::HandleInteract, pkt);

	return true;
}
