#pragma once

#include "Room.h"
#include "Singleton.h"

class RoomManager : public Singleton<RoomManager>
{
public:
	void CreateRoom(shared_ptr<Player> player, const std::string& name, const std::string map);
	void EnterRoom(shared_ptr<Player> player, const Protocol::C_ENTER_ROOM& pkt);

	void EnterGame(int64 roomID);

	void UpdateRoom();


public:
	const std::unordered_map<int, shared_ptr<Room>>& RoomList();

private:
	USE_LOCK;

	std::unordered_map<int, shared_ptr<Room>> _rooms;
};

