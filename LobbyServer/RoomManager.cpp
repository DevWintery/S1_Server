#include "pch.h"
#include "RoomManager.h"

#include <cpprest/http_listener.h>              // HTTP server
#include <cpprest/uri.h>                        // URI library
#include <cpprest/containerstream.h>            // Async streams backed by STL containers
#include <cpprest/interopstream.h>              // Bridges for integrating Async streams with STL and WinRT streams
#include <cpprest/rawptrstream.h>               // Async streams backed by raw pointer to memory
#include <cpprest/producerconsumerstream.h>     // Async streams for producer consumer scenarios
#include <cpprest/http_client.h>
#include <shellapi.h>

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

using namespace web::http::experimental::listener;          // HTTP server
using namespace web::json;                                  // JSON library
RoomManager::RoomManager():
	nextRoomId(1)
{

}

int RoomManager::CreateRoom(const std::wstring& name)
{
	std::lock_guard<std::mutex> lock(mtx);
	int roomId = nextRoomId++;
	rooms[roomId] = Room(roomId, name);
	return roomId;
}

web::json::value RoomManager::ListRooms()
{
	std::lock_guard<std::mutex> lock(mtx);
	web::json::value result = web::json::value::array();
	int index = 0;
	for (auto& pair : rooms) 
	{
		result.as_array()[index++] = pair.second.to_json();
	}
	return result;
}

bool RoomManager::JoinRoom(int roomId, const std::wstring& playerName)
{
	std::lock_guard<std::mutex> lock(mtx);
	auto it = rooms.find(roomId);
	if (it != rooms.end() && !it->second.isGameStarted) 
	{
		it->second.players.push_back(playerName);
		return true;
	}
	return false;
}

pplx::task<bool> RoomManager::StartGame(int roomId, const std::wstring& args)
{
	std::lock_guard<std::mutex> lock(mtx);
	auto it = rooms.find(roomId);
	if (it != rooms.end() && !it->second.isGameStarted) 
	{
		it->second.isGameStarted = true;

		WCHAR path[MAX_PATH];
		if (GetModuleFileName(NULL, path, MAX_PATH) == 0) 
		{
			std::cerr << "Error getting module file name." << std::endl;
			return pplx::task_from_result(false);
		}

		WCHAR* lastSlash = wcsrchr(path, '\\');
		if (lastSlash == NULL) 
		{
			std::cerr << "Error finding the last backslash." << std::endl;
			return pplx::task_from_result(false);
		}
		*(lastSlash + 1) = L'\0';  // 마지막 '\' 이후 문자를 NULL로 설정하여 경로를 종료합니다.

		// 'Server.exe'를 경로에 추가합니다.
		wcscat_s(path, MAX_PATH, L"Server.exe");

		ShellExecute(0, L"open", path, args.c_str(), NULL, SW_SHOW);

		return pplx::task_from_result(true);
	}
	return pplx::task_from_result(false);
}