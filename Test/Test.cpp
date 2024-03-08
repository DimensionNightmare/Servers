#include <iostream>
#ifdef _WIN32
	#include <coroutine>
#elif __linux__
	#include <coroutine>
#endif
#include <map>
#include <unordered_map>
#include <fstream>
#include "GCfg.pb.h"
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

using namespace hv;

using namespace std;

class A
{
public:
	A() {}
	// virtual
	~A() {}

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