#include <iostream>
#include <coroutine>
#include <map>
#include <unordered_map>
#include <fstream>
#include "schema.pb.h"
#include <Windows.h>
#include <locale>
#include <random>

class DNClientProxy {
public:
    template <typename... Args>
    void RegistSelf(Args... args);
};

template <typename... Args>
void DNClientProxy::RegistSelf(Args... args) {
    ((std::cout << args << " "), ...);
    std::cout << std::endl;
}

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
	
	std::random_device rd;
    std::mt19937 gen(rd());
	std::bernoulli_distribution  u;
	for(int i = 0; i < 5; i++)
	std::cout << u(gen) << std::endl;

	DNClientProxy* res = new DNClientProxy;
	res->RegistSelf<int,int,int,int,int>(3,5,6,8,7);

    return 0;
}
