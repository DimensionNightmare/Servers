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
#include "sw/redis++/redis++.h"

#include "GCfg/GCfg.pb.h"

using namespace hv;
using namespace std;
using namespace sw::redis;

// #define TIMERSTART(tag) auto tag##_start = chrono::steady_clock::now(),tag##_end = tag##_start
// #define TIMEREND(tag) tag##_end = chrono::steady_clock::now()
// #define DURATION_s(tag) DNPrint(0, LoggerLevel::Debug, "%s costs %d s\n",#tag,chrono::duration_cast<chrono::seconds>(tag##_end - tag##_start).count())
// #define DURATION_ms(tag) DNPrint(0, LoggerLevel::Debug, "%s costs %d ms\n",#tag,chrono::duration_cast<chrono::milliseconds>(tag##_end - tag##_start).count());
// #define DURATION_us(tag) DNPrint(0, LoggerLevel::Debug, "%s costs %d us\n",#tag,chrono::duration_cast<chrono::microseconds>(tag##_end - tag##_start).count());
// #define DURATION_ns(tag) DNPrint(0, LoggerLevel::Debug, "%s costs %d ns\n",#tag,chrono::duration_cast<chrono::nanoseconds>(tag##_end - tag##_start).count());


#if 0
int main() 
{
	GCfg::CharacterPlayer Weapons;
	ifstream input("C:\\Project\\DimensionNightmare\\Environment\\GameConfig\\Gen\\Data\\character_player.bytes", ios::in | ios::binary);
	if(input)
	{
		if(Weapons.ParseFromIstream(&input))
		{
			// Weapons.Clear();
			auto map = Weapons.data_map();
			for(auto one:map)
			{
				cout << "key" << one.first << endl;
				cout << "value"<< one.second.DebugString() << endl;
			}
			auto find = map.find(55);
			cout << "success" << endl;
		}
		else
		{
			cout << "error" << endl;
		}

	}
	
	random_device rd;
    mt19937 gen(rd());
	bernoulli_distribution  u;
	for(int i = 0; i < 5; i++)
	cout << u(gen) << endl;

	string msgName = GCfg::CharacterPlayer::GetDescriptor()->full_name();
	cout << msgName.size() << " " << msgName.length() << " " <<  strlen(msgName.c_str()) << endl;
	auto hashres = hash<string>::_Do_hash.operator()("");
	cout << size_t(hashres) << " " <<  hashres << endl;

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
		ifstream input("/home/DimensionNightmare/Environment/GameConfig/Gen/Data/character_player.bytes", ios::in | ios::binary);
		if(input)
		{
			if(Weapons.ParseFromIstream(&input))
			{
				cout << "success" << endl;
			}
			else
			{
				cout << "error" << endl;
			}
		}
		else
		{

		}
	}

	auto item = Weapons.data_map().find(1);
	if(item != Weapons.data_map().end())
	{
		const GCfg::PlayerInfo * info = &item->second;
		// info->clear_type();
		const string& name = info->name();
		// name.empty();
		// info->set_type(GCfg::NTypeCharacterPlayer_Normal);
	}
}
#endif

chrono::hours GetTimezoneOffset()
{
	int minutes = 0;
#ifdef _win32
	TIME_ZONE_INFORMATION timeZoneInfo;
    DWORD result = GetTimeZoneInformation(&timeZoneInfo);

	if (result != TIME_ZONE_ID_INVALID) {
		minutes = -timeZoneInfo.Bias;
		wcout.imbue(locale("zh_CN.UTF-8"));
        wcout << "Standard Name: " << timeZoneInfo.StandardName << endl;
        wcout << "Daylight Name: " << timeZoneInfo.DaylightName << endl;
    } else {
        cerr << "Failed to get time zone information." << endl;
    }
#endif

	return chrono::hours(minutes/60);
}

void printTime()
{
	cout << format("{:%Y-%m-%d %H:%M:%S}", chrono::system_clock::now()) << endl;

	static chrono::hours offset = GetTimezoneOffset();
	
	cout << format("{:%Y-%m-%d %H:%M:%S}", chrono::system_clock::now() + offset) << endl;
}

#if 0
int main()
{
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
	// TIMERSTART(for_loop);
	
	printTime();

	// TIMEREND(for_loop);
	// DURATION_ms(for_loop);

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

#if 1 
int main()
{
	ConnectionOptions connection_options;
    connection_options.host = "127.0.0.1";  // Required.
    connection_options.port = 6379; // Optional. The default port is 6379.
    //connection_options.password = "auth";   // Optional. No password by default.
 
    ConnectionPoolOptions pool_options;
    pool_options.size = 3;  // Pool size, i.e. max number of connections.
    pool_options.wait_timeout = chrono::milliseconds(100);

	try
	{
		Redis* con = new Redis(connection_options, pool_options);
		con->ping();
		map<string, string> hashTerm;
    	con->hgetall("*",inserter(hashTerm, hashTerm.end()));

		for (const auto &[k, v] : hashTerm)
		{
    		cout << "m[" << k << "] = (" << v << ") " << endl;
		}
	}
	catch(const exception& e)
	{
		cout << e.what() << endl;
	}
}
#endif