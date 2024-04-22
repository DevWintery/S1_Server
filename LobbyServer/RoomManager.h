#pragma once

#include <unordered_map>
#include "Room.h"

class RoomManager 
{

public:
	RoomManager();

public:
	int CreateRoom(const std::wstring& name);
	web::json::value ListRooms();
	bool JoinRoom(int roomId, const std::wstring& playerName);
	pplx::task<bool> StartGame(int roomId, const std::wstring& args);

private:
	std::unordered_map<int, Room> rooms;
	std::atomic_int nextRoomId;
	std::mutex mtx;
};