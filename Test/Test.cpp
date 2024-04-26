#include <iostream>
#include <coroutine>
#include <map>
#include <unordered_map>
#include <fstream>
#include <random>
#include <string>
#include <thread>
#include <chrono>
// #include <Windows.h>
#include "hv/TcpClient.h"
#include "hv/EventLoop.h"
#include "hv/hloop.h" 
#include "hv/requests.h"
#include "hv/json.hpp"
#include "pqxx/pqxx"

#include "GCfg/GCfg.pb.h"

using namespace hv;
using namespace std;

// #define TIMERSTART(tag) auto tag##_start = std::chrono::steady_clock::now(),tag##_end = tag##_start
// #define TIMEREND(tag) tag##_end = std::chrono::steady_clock::now()
// #define DURATION_s(tag) DNPrint(0, LoggerLevel::Debug, "%s costs %d s\n",#tag,std::chrono::duration_cast<std::chrono::seconds>(tag##_end - tag##_start).count())
// #define DURATION_ms(tag) DNPrint(0, LoggerLevel::Debug, "%s costs %d ms\n",#tag,std::chrono::duration_cast<std::chrono::milliseconds>(tag##_end - tag##_start).count());
// #define DURATION_us(tag) DNPrint(0, LoggerLevel::Debug, "%s costs %d us\n",#tag,std::chrono::duration_cast<std::chrono::microseconds>(tag##_end - tag##_start).count());
// #define DURATION_ns(tag) DNPrint(0, LoggerLevel::Debug, "%s costs %d ns\n",#tag,std::chrono::duration_cast<std::chrono::nanoseconds>(tag##_end - tag##_start).count());


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

	std::string msgName = GCfg::CharacterPlayer::GetDescriptor()->full_name();
	std::cout << msgName.size() << " " << msgName.length() << " " <<  strlen(msgName.c_str()) << std::endl;
	auto hashres = std::hash<string>::_Do_hash.operator()("");
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

chrono::hours GetTimezoneOffset()
{
	int minutes = 0;

	TIME_ZONE_INFORMATION timeZoneInfo;
    DWORD result = GetTimeZoneInformation(&timeZoneInfo);

	if (result != TIME_ZONE_ID_INVALID) {
		minutes = -timeZoneInfo.Bias;
		std::wcout.imbue(locale("zh_CN.UTF-8"));
        std::wcout << "Standard Name: " << timeZoneInfo.StandardName << std::endl;
        std::wcout << "Daylight Name: " << timeZoneInfo.DaylightName << std::endl;
    } else {
        std::cerr << "Failed to get time zone information." << std::endl;
    }

	return chrono::hours(minutes/60);
}

void printTime()
{
	cout << format("{:%Y-%m-%d %H:%M:%S}", chrono::system_clock::now()) << endl;

	static chrono::hours offset = GetTimezoneOffset();
	
	cout << format("{:%Y-%m-%d %H:%M:%S}", chrono::system_clock::now() + offset) << endl;
}

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

	time_t timep;

    time(&timep);
    printf("%s\n", ctime(&timep));

	// chrono::system_clock clock;

	// chrono::system_clock time_;
	// cout << format("{:%Y-%m-%d %H:%M:%S}", time_.now()) << endl;

	//  time_;
	TIMERSTART(for_loop);
	
	printTime();

	TIMEREND(for_loop);
	DURATION_ms(for_loop);

	// TIMERSTART(for_loop);
	
	// printTime();

	// TIMEREND(for_loop);
	// DURATION_ms(for_loop);
	// printTime();
	// wstring msg((wchar_t*)format("[{}] {} -> \n{}", "哈哈", "asdasd", "zc").c_str());
	// wcout <<  msg << endl;

    
    return 0;
}


#endif