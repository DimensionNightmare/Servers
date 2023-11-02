#include <iostream>
#include <coroutine>
#include <map>
#include <unordered_map>
#include <fstream>
#include "schema.pb.h"


int main() 
{
	GCfg::CharacterPlayer Weapons;
	 std::ifstream input("D:\\Project\\DimensionNightmare\\Environment\\GameConfig\\Gen\\Data\\character_player.bytes", std::ios::in | std::ios::binary);
	 if(input)
	 {
		if(Weapons.ParseFromIstream(&input))
		{
			Weapons.Clear();
			auto map = Weapons.data_map();
			for(auto one:map)
			{
				std::cout << "key" << one.first << std::endl;
				std::cout << "value"<< one.second.name() << std::endl;
			}
			auto find = map.find(55);
			std::cout << "success" << std::endl;
		}

	 }
    return 0;
}
