#include "pch.h"
#include "RoomManager.h"
#include <cpprest/http_listener.h>
#include <sstream>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

int main(int argc, char* argv[])
{
	RoomManager manager;

	std::size_t len = std::strlen(argv[1]);
	std::wstring wstr(len, L' ');  // 충분한 크기의 와이드 문자열을 생성합니다.
	std::mbstowcs(&wstr[0], argv[1], len);  // 멀티바이트를 와이드 문자로 변환합니다.

	//uri_builder uri(U("http://localhost:34568"));
	uri_builder uri(wstr);
	auto addr = uri.to_uri().to_string();
	http_listener listener(addr);

	listener.support(methods::POST, [&manager](http_request request)
		{
			// 방 생성에 대한 POST 요청
			if (request.relative_uri().path() == U("/rooms"))
			{
				request.extract_json().then([&manager, request](json::value json_val)
					{
						auto room_name = json_val[U("name")].as_string();
						int room_id = manager.CreateRoom(room_name);
						json::value response_data;
						response_data[U("room_id")] = json::value::number(room_id);
						return request.reply(status_codes::OK, response_data);
					}).then([](pplx::task<void> t)
						{
							try
							{
								t.get();
							}
							catch (const std::exception& e)
							{
								std::cerr << "An error occurred: " << e.what() << std::endl;
							}
						});
			}

			//방 시작에 대한 POST 요청
			if (request.relative_uri().path() == U("/startRoom"))
			{
				request.extract_json().then([&manager, request](json::value json_val)
					{
						auto room_id = json_val[U("id")].as_integer();
						std::wstring args = json_val[U("args")].as_string();
						bool result = manager.StartGame(room_id, args).wait();

						//args 규칙
						//0 - ip
						//1 - map Name

						std::wstringstream ss(args);
						std::wstring ip, mapName;

						ss >> ip >> mapName;

						json::value response_data;
						response_data[U("sucess")] = json::value::boolean(result);
						response_data[U("connect-ip")] = json::value::string(ip);
						response_data[U("connect-port")] = json::value::number(7777);
						response_data[U("map-name")] = json::value::string(mapName);
						return request.reply(status_codes::OK, response_data);
					}).then([](pplx::task<void> t)
						{
							try
							{
								t.get();
							}
							catch (const std::exception& e)
							{
								std::wcout << L"An error occurred: " << e.what() << std::endl;
							}
						});
			}
		});

	// 방 조회에 대한 GET 요청
	listener.support(methods::GET, [&manager](http_request request)
		{
			if (request.relative_uri().path() == U("/rooms"))
			{
				auto response_data = manager.ListRooms();
				request.reply(status_codes::OK, response_data);
			}
		});

	try
	{
		listener
			.open()
			.then([&listener]() { std::wcout << "Starting to listen at: " << listener.uri().to_string() << std::endl; })
			.wait();

		std::string line;
		std::cout << "Enter 'exit' to quit." << std::endl;
		do
		{
			std::getline(std::cin, line);
		} while (line != "exit");

		listener
			.close()
			.then([]() { std::cout << "Stopped listening." << std::endl; })
			.wait();
	}
	catch (const std::exception& e)
	{
		std::cerr << "An error occurred: " << e.what() << std::endl;
		return -1;
	}
}
