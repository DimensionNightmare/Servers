#include <iostream>
#ifdef _WIN32
	#include <coroutine>
#elif __linux__
	#include <coroutine>
#endif
#include <map>
#include <unordered_map>
#include <fstream>
#include "GCfg/GCfg.pb.h"
// #include <Windows.h>
#include <locale>
#include <random>
#include "hv/TcpClient.h"
#include <string>
#include "hv/EventLoop.h"
#include "hv/hloop.h" 
#include "hv/requests.h"
#include <signal.h>
#include <thread>
#include <chrono>
#include "pqxx/pqxx"
#include "hv/json.hpp"

using namespace hv;

using namespace std;

class A
{
public:
	A() {std::cout << "A" << std::endl;}
	// virtual
	~A() {std::cout << "~A" << std::endl;}

public:
	 void msg() { std::cout << "A" << std::endl; }

private:
	int a;
};

class B : public A
{
public:
B() {}
	// virtual
	~B() {}
	 void msg() { std::cout << "B" << std::endl; }
};

#if 0
int main() 
{
	GCfg::CharacterPlayer Weapons;
	std::ifstream input("C:\\Project\\DimensionNightmare\\Environment\\GameConfig\\Gen\\Data\\character_player.bytes", std::ios::in | std::ios::binary);
	if(input)
	{
		if(Weapons.ParseFromIstream(&input))
		{
			// Weapons.Clear();
			auto map = Weapons.data_map();
			for(auto one:map)
			{
				std::cout << "key" << one.first << std::endl;
				std::cout << "value"<< one.second.DebugString() << std::endl;
			}
			auto find = map.find(55);
			std::cout << "success" << std::endl;
		}
		else
		{
			std::cout << "error" << std::endl;
		}

	}
	
	std::random_device rd;
    std::mt19937 gen(rd());
	std::bernoulli_distribution  u;
	for(int i = 0; i < 5; i++)
	std::cout << u(gen) << std::endl;

	std::hash<std::string> hashstr;

	std::string msgName = GCfg::CharacterPlayer::GetDescriptor()->full_name();
	std::cout << msgName.size() << " " << msgName.length() << " " <<  strlen(msgName.c_str()) << std::endl;
	auto hashres = hashstr.operator()("");
	std::cout << size_t(hashres) << " " <<  hashres << std::endl;

	using namespace std;
	A a;
	B* b = (B*)&a;
	b->msg();
	B c;
	// c.msg();
	A* d = &c;
	d->msg();
	cout << sizeof(a) << endl;

    return 0;
}
#endif

#if 0
int main()
{
	GCfg::CharacterPlayer Weapons;
	{
		std::ifstream input("C:\\Project\\DimensionNightmare\\Environment\\GameConfig\\Gen\\Data\\character_player.bytes", std::ios::in | std::ios::binary);
		if(input)
		{
			if(Weapons.ParseFromIstream(&input))
			{
				std::cout << "success" << std::endl;
			}
			else
			{
				std::cout << "error" << std::endl;
			}
		}
	}

	auto item = Weapons.data_map().find(0);
	if(item != Weapons.data_map().end())
	{
		const GCfg::PlayerInfo * info = &item->second;
		// info->clear_type();
		const std::string& name = info->name();
		// name.empty();
		// info->set_type(GCfg::NTypeCharacterPlayer_Normal);
	}
}
#endif

#if 1
int main()
{
	std::map<int, A> maps;

	A* a = &maps[1];
	maps.erase(1);

	string jsonstr = R"(
	{
		"aa": 1,
		"bb":{
			"aa" :2,
			"cc":[{"dd":1},2,3]
		}
	}
	)";

	nlohmann::json j = nlohmann::json::parse(jsonstr);

	if(j.contains("/bb/cc/2"_json_pointer))
	{
		printf("1");
	}
	else
	{
		printf("0");
	}

	function<void(int)> pOnClose;

	if(pOnClose)
	{
		printf("has func");
	}
}
#endif