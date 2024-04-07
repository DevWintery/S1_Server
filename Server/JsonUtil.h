#pragma once

#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

namespace ns
{
	struct Location
	{
		float x, y, z;
	};

	struct Monster
	{
		std::string type;
		Location location;
		std::optional<Location> dest_location;
	};

	static void from_json(const json& j, Location& loc)
	{
		j.at("x").get_to(loc.x);
		j.at("y").get_to(loc.y);
		j.at("z").get_to(loc.z);
	}

	static void from_json(const json& j, Monster& monster)
	{
		monster.type = j.at("type").get<std::string>();
		monster.location = j.at("location").get<Location>();
		if (j.contains("dest_location"))
		{
			monster.dest_location = j.at("dest_location").get<Location>();
		}
	}
}


class JsonUtil
{
public:
	static std::vector<ns::Monster> ParseJson(std::string fileName)
	{
		json j;
		ParseJson(j, fileName);

		std::vector<ns::Monster> result;

		for (const auto& step : j["scenario_steps"]) 
		{
			std::cout << "Step: " << step["step"] << std::endl;
			for (const auto& monster_json : step["monsters"]) 
			{
				ns::Monster monster = monster_json.get<ns::Monster>();
				std::cout << "  Monster Type: " << monster.type << std::endl;
				std::cout << "  Location: (" << monster.location.x << ", " << monster.location.y << ", " << monster.location.z << ")" << std::endl;
				if (monster.dest_location) 
				{
					std::cout << "  Destination Location: (" << monster.dest_location->x << ", " << monster.dest_location->y << ", " << monster.dest_location->z << ")" << std::endl;
				}

				result.push_back(monster);
			}
		}

		return result;
	}

	static std::vector<ns::Monster> ParseJson(std::string fileName, int stepID)
	{
		json j;
		ParseJson(j, fileName);

		std::vector<ns::Monster> result;

		for (const auto& step : j["scenario_steps"])
		{
			if (step["step"] != stepID)
			{
				continue;
			}

			std::cout << "Step: " << step["step"] << std::endl;
			for (const auto& monster_json : step["monsters"])
			{
				ns::Monster monster = monster_json.get<ns::Monster>();
				std::cout << "  Monster Type: " << monster.type << std::endl;
				std::cout << "  Location: (" << monster.location.x << ", " << monster.location.y << ", " << monster.location.z << ")" << std::endl;
				if (monster.dest_location)
				{
					std::cout << "  Destination Location: (" << monster.dest_location->x << ", " << monster.dest_location->y << ", " << monster.dest_location->z << ")" << std::endl;
				}

				result.push_back(monster);
			}
		}

		return result;
	}

private:
	static void ParseJson(json& j, std::string fileName)
	{
		std::ifstream file(fileName);

		if (!file.is_open())
		{
			std::cout << "파일을 열 수 없습니다." << std::endl;
			return;
		}

		file >> j;
	}
};

