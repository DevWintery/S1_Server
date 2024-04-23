#pragma once

#include <cpprest/json.h>

class Room
{
public:
	Room() {}
	Room(int id, const std::wstring& name, const std::wstring& player_name, const std::wstring& ip);

	web::json::value to_json() const;

public:
	int id;
	std::wstring name;
	std::wstring ip;
	std::vector<std::wstring> players;
};