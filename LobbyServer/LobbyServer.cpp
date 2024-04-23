#include "pch.h"
#include <cpprest/http_listener.h>
#include <sstream>
#include "RoomManager.h"

RoomManager GManager;

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;


// �� ������ ���� POST ��û
pplx::task<void> PostCreateRoom(http_request request)
{
	request.extract_json().then([request](json::value json_val)
		{
			std::wstring room_name = json_val[U("name")].as_string();
			std::wstring map_name = json_val[U("map_name")].as_string();
			std::wstring player_name = json_val[U("player_name")].as_string();
			std::wstring room_ip = json_val[U("room_ip")].as_string();

			int room_id = GManager.CreateRoom(room_name, player_name, room_ip);

			std::wstring args = room_ip + L" " + map_name;
			GManager.StartRoom(room_id, args);

			json::value response_data;
			response_data[U("room_id")] = json::value::number(room_id);
			return request.reply(status_codes::OK, response_data);
		}).wait();

	return pplx::task_from_result();
}

pplx::task<void> PostJoinRoom(http_request request)
{
	request.extract_json().then([request](json::value json_val)
		{
			int room_id = json_val[U("id")].as_integer();
			std::wstring player_name = json_val[U("player_name")].as_string();

			if (GManager.JoinRoom(room_id, player_name) == false)
			{
				return pplx::task_from_result();
			}

			json::value response_data;
			response_data[U("successed")] = json::value::boolean(true);
			return request.reply(status_codes::OK, response_data);
		}).wait();

	return pplx::task_from_result();
}

pplx::task<void> PostHandler(http_request request)
{
	if (request.relative_uri().path() == U("/rooms"))
	{
		PostCreateRoom(request);
	}

	if (request.relative_uri().path() == U("/join"))
	{
		PostJoinRoom(request);
	}

	return pplx::task_from_result();
}

// �� ��ȸ�� ���� GET ��û
pplx::task<void> GetRooms(http_request request)
{
	if (request.relative_uri().path() == U("/rooms"))
	{
		auto response_data = GManager.ListRooms();
		request.reply(status_codes::OK, response_data);
	}

	return pplx::task_from_result();
}

int main(int argc, char* argv[])
{
	std::size_t len = std::strlen(argv[1]);
	std::wstring wstr(len, L' ');  // ����� ũ���� ���̵� ���ڿ��� �����մϴ�.
	std::mbstowcs(&wstr[0], argv[1], len);  // ��Ƽ����Ʈ�� ���̵� ���ڷ� ��ȯ�մϴ�.

	//uri_builder uri(U("http://localhost:34568"));
	uri_builder uri(wstr);
	auto addr = uri.to_uri().to_string();
	http_listener listener(addr);

	listener.support(methods::POST, PostHandler);
	listener.support(methods::GET, GetRooms);

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
