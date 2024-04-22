#pragma once

#include <cpprest/json.h>

class Room
{
public:
	Room() {}
	Room(int id, const std::wstring& name);

	web::json::value to_json() const;

public:
	int id;
	std::wstring name;
	bool isGameStarted;
	std::vector<std::wstring> players;
};