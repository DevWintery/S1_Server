#pragma once

#include <unordered_map>
#include "Room.h"

class RoomManager 
{

public:
	RoomManager();

public:
	int CreateRoom(const std::wstring& name, const std::wstring& player_name, const std::wstring& room_ip);
	web::json::value ListRooms();
	bool JoinRoom(int roomId, const std::wstring& playerName);
	pplx::task<bool> StartRoom(int roomId, const std::wstring& args);

private:
	std::unordered_map<int, Room> rooms;
	std::atomic_int nextRoomId;
	std::mutex mtx;
};