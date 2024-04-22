#include "pch.h"
#include "Room.h"

Room::Room(int id, const std::wstring& name) : 
	id(id), name(name), isGameStarted(false)
{
}

web::json::value Room::to_json() const
{
	web::json::value result = web::json::value::object();
	result[U("id")] = web::json::value::number(id);
	result[U("name")] = web::json::value::string(name);
	result[U("isGameStarted")] = web::json::value::boolean(isGameStarted);

	web::json::value playerArray = web::json::value::array(players.size());
	for (size_t i = 0; i < players.size(); ++i) 
	{
		playerArray[i] = web::json::value::string(players[i]);
	}
	result[U("players")] = playerArray;

	return result;
}
