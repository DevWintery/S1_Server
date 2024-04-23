#include "pch.h"
#include "Room.h"

Room::Room(int id, const std::wstring& name, const std::wstring& player_name, const std::wstring& ip) :
	id(id), name(name), ip(ip)
{
	players.push_back(player_name);
}

web::json::value Room::to_json() const
{
	web::json::value result = web::json::value::object();
	result[U("id")] = web::json::value::number(id);
	result[U("name")] = web::json::value::string(name);
	result[U("room_ip")] = web::json::value::string(ip);
	
	web::json::value playerArray = web::json::value::array(players.size());
	for (size_t i = 0; i < players.size(); ++i) 
	{
		playerArray[i] = web::json::value::string(players[i]);
	}
	result[U("players")] = playerArray;

	return result;
}
