#include "pch.h"
#include "RoomManager.h"
#include "ObjectUtils.h"

void RoomManager::CreateRoom(shared_ptr<Player> player, const std::string& name, const std::string map)
{
	int64 id = ObjectUtils::GetID();
	shared_ptr<Room> room = make_shared<Room>(name, map);

	{
		WRITE_LOCK;
		_rooms.insert(make_pair(id, room));
	}

	room->DoAsync(&Room::EnterRoom, player, id);
}

void RoomManager::EnterRoom(shared_ptr<Player> player, const Protocol::C_ENTER_ROOM& pkt)
{
	int64 id = pkt.room_id();
	if (_rooms.find(id) == _rooms.end()) //없으면 문제가 있음
	{
		return;
	}

	_rooms[id]->DoAsync(&Room::EnterRoom, player, id);
}

void RoomManager::EnterGame(int64 roomID)
{
	if (_rooms.find(roomID) == _rooms.end()) //없으면 문제가 있음
	{
		return;
	}

	_rooms[roomID]->DoAsync(&Room::EnterGame);
	_rooms[roomID]->DoAsync(&Room::UpdateRoom);
}

void RoomManager::UpdateRoom()
{
	for (const auto& room : _rooms)
	{
	}
}

const std::unordered_map<int, shared_ptr<Room>>& RoomManager::RoomList()
{
	return _rooms;
}
