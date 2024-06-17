#include <iostream>
#include <coroutine>
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

#undef REPEATED
#include "google/protobuf/util/json_util.h"

#include "GCfg/GCfg.pb.h"
#include "GDef/GDef.pb.h"

using namespace hv;
using namespace std;
using namespace sw::redis;
using namespace GDb;
using namespace google::protobuf;

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
	if (input)
	{
		if (Weapons.ParseFromIstream(&input))
		{
			// Weapons.Clear();
			auto map = Weapons.data_map();
			for (auto one : map)
			{
				cout << "key" << one.first << endl;
				cout << "value" << one.second.DebugString() << endl;
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
	for (int i = 0; i < 5; i++)
		cout << u(gen) << endl;

	string msgName = GCfg::CharacterPlayer::GetDescriptor()->full_name();
	cout << msgName.size() << " " << msgName.length() << " " << strlen(msgName.c_str()) << endl;
	auto hashres = hash<string>::_Do_hash.operator()("");
	cout << size_t(hashres) << " " << hashres << endl;

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
		if (input)
		{
			if (Weapons.ParseFromIstream(&input))
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
	if (item != Weapons.data_map().end())
	{
		const GCfg::PlayerInfo* info = &item->second;
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

	if (result != TIME_ZONE_ID_INVALID)
	{
		minutes = -timeZoneInfo.Bias;
		wcout.imbue(locale("zh_CN.UTF-8"));
		wcout << "Standard Name: " << timeZoneInfo.StandardName << endl;
		wcout << "Daylight Name: " << timeZoneInfo.DaylightName << endl;
	}
	else
	{
		cerr << "Failed to get time zone information." << endl;
	}
#endif

	return chrono::hours(minutes / 60);
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

	if (j.contains("/bb/cc/2"_json_pointer))
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

#if 0 
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
		unordered_map<string, string> hashTerm;
		con->hgetall("*", inserter(hashTerm, hashTerm.end()));

		for (const auto& [k, v] : hashTerm)
		{
			cout << "m[" << k << "] = (" << v << ") " << endl;
		}
	}
	catch (const exception& e)
	{
		cout << e.what() << endl;
	}

	std::ios_base::sync_with_stdio(false);

	std::cin.tie(nullptr);
	std::cout.tie(nullptr);

	// std::cin.sync_with_stdio(false);

	while (true)
	{
		if (std::cin.peek() != EOF)
		{
			string userInput;
			std::cin >> userInput;
			std::cout << "You entered: " << userInput << std::endl;

			std::cin.ignore();
		}

		std::cout << "Doing something else..." << std::endl;
	}
}
#endif

void BytesToHexString(string& bytes)
{
	std::ostringstream oss;
	oss << std::hex << std::setfill('0');
	for (unsigned char byte : bytes)
	{
		oss << std::setw(2) << static_cast<int>(byte);
	}
	bytes = oss.str();
}

void HexStringToBytes(string& hexString)
{
	string byteString = hexString;
	hexString.clear();
	for (size_t i = 0; i < byteString.length(); i += 2)
	{
		hexString += static_cast<unsigned char>(std::stoi(byteString.substr(i, 2), nullptr, 16));
	}
}

#if 0

int main()
{
	Player player;
	player.set_account_id(11);
	PropertyEntity* propertyEntity = player.mutable_property_entity();
	propertyEntity->set_hp_max(1);
	propertyEntity->set_mp_max(1);
	propertyEntity->set_attack(1);
	propertyEntity->set_defense(1);
	string msgData;
	msgData = "asdasda";
	player.SerializeToString(&msgData);

	BytesToHexString(msgData);

	std::cout << "Hex string: " << msgData << std::endl;

	HexStringToBytes(msgData);

	std::cout << "Bytes: " << msgData << std::endl;
	player.Clear();
	player.ParseFromString(msgData);
	string msgData1;
	util::MessageToJsonString(player, &msgData1);

	std::cout << "Serlize: " << msgData1 << std::endl;

	player.Clear();
	util::JsonStringToMessage(msgData1, &player);

	std::cout << "id: " << player.account_id() << std::endl;
}
#endif

template <typename T>
struct DNTask
{
	struct promise_type;
	using HandleType = coroutine_handle<promise_type>;
	struct promise_type
	{
		promise_type()
		{
		}

		DNTask get_return_object()
		{
			return DNTask{ HandleType::from_promise(*this) };
		}

		void return_value(const T& value)
		{
			oResult = &value;
			bReturned = true;
		}

		suspend_always initial_suspend() { return {}; }

		suspend_always final_suspend() noexcept
		{
			// DNTask don't Call by self, need Message handle Tick;
			// ReleaseAwaitHandle();
			return {};
		}

		void unhandled_exception() {}

		const T& GetResult() const { return *oResult; }

		void ReleaseAwaitHandle()
		{
			if (oAwaitHandle) { oAwaitHandle.resume(); oAwaitHandle = nullptr; }
		}

		const T* oResult = nullptr;

		coroutine_handle<> oAwaitHandle = nullptr;

		bool bReturned = false;
	};

	// Awaitable
	bool await_ready() const noexcept
	{
		return tHandle.promise().bReturned;
	}

	void await_suspend(coroutine_handle<> caller)
	{
		tHandle.promise().oAwaitHandle = caller;

	
	}

	void await_resume() noexcept
	{
	}
	// Awaitable

	DNTask(HandleType handle)
	{
		tHandle = handle;
		// SetFlag(DNTaskFlag::TimeCost);
	}

	~DNTask()
	{
		Destroy();
	}

	void Resume()
	{
		if (!tHandle || tHandle.done())
		{
			return;
		}

		tHandle.resume();
	}

	void CallResume()
	{
		tHandle.promise().ReleaseAwaitHandle();
	}

	const T& GetResult()
	{
		return tHandle.promise().GetResult();
	}

	void Destroy()
	{
	

		if (tHandle)
		{
			tHandle.destroy();
			tHandle = nullptr;
		}
	}
public:


private:
	HandleType tHandle;


};

struct DNTaskVoid
{
	struct promise_type;
	using HandleType = coroutine_handle<promise_type>;
	struct promise_type
	{
		promise_type() {}

		void return_void() { bReturned = true; }

		DNTaskVoid get_return_object()
		{
			return DNTaskVoid{ HandleType::from_promise(*this) };
		}

		suspend_never initial_suspend() { return {}; }

		suspend_never final_suspend() noexcept
		{
			ReleaseAwaitHandle();
			return {};
		}

		void unhandled_exception() {}

		void ReleaseAwaitHandle()
		{
			if (oAwaitHandle) { oAwaitHandle.resume(); oAwaitHandle = nullptr; }
		}

		coroutine_handle<> oAwaitHandle = nullptr;

		bool bReturned = false;
	};

	// Awaitable Start
	bool await_ready() const noexcept
	{
		return tHandle.promise().bReturned;
	}

	void await_suspend(coroutine_handle<> caller)
	{
		tHandle.promise().oAwaitHandle = caller;
	}

	void await_resume() noexcept
	{
	}
	// Awaitable End

	DNTaskVoid(HandleType handle)
	{
		tHandle = handle;
	}

	void Resume()
	{
		if (!tHandle || tHandle.done())
		{
			return;
		}

		tHandle.resume();
	}

	HandleType tHandle;
};



class TimerThread : public EventLoopThread {
public:
    std::atomic<TimerID> nextTimerID;
    TimerThread() : EventLoopThread() {
        nextTimerID = 0;
        start();
    }

    virtual ~TimerThread() {
        stop();
        join();
    }

public:
    // setTimer, setTimeout, killTimer, resetTimer thread-safe
    TimerID setTimer(int timeout_ms, TimerCallback cb, uint32_t repeat = INFINITE) {
        TimerID timerID = ++nextTimerID;
        loop()->setTimerInLoop(timeout_ms, cb, repeat, timerID);
        return timerID;
    }
    // alias javascript setTimeout, setInterval
    TimerID setTimeout(int timeout_ms, TimerCallback cb) {
        return setTimer(timeout_ms, cb, 1);
    }
    TimerID setInterval(int interval_ms, TimerCallback cb) {
        return setTimer(interval_ms, cb, INFINITE);
    }

    void killTimer(TimerID timerID) {
        loop()->killTimer(timerID);
    }

    void resetTimer(TimerID timerID, int timeout_ms = 0) {
        loop()->resetTimer(timerID, timeout_ms);
    }
};

shared_ptr<TimerThread> loop;

DNTaskVoid funcD() {
    std::cout << "1" << std::endl;
	co_return;
}

DNTask<int*> funcC() {
    std::cout << "1" << std::endl;
	int* a = new int();
	co_return a;
}

DNTaskVoid funcB() {
	auto res = funcC();
	
	loop->setTimeout(2500, [&](int64_t timeId){
		res.CallResume();
	});
	
    co_await res;

	// co_await funcD();
	
    std::cout << "2" << std::endl;
}

DNTaskVoid funcA() {
    co_await funcB();
	co_await funcD();
	co_await funcB();
	co_await funcD();
	co_await funcB();
    std::cout << "3" << std::endl;
}

int main() {
	loop = make_shared<TimerThread>();
    funcA();

    std::this_thread::sleep_for(std::chrono::seconds(20));  // 确保主线程在协程执行完之前不会退出

	loop->stop(true);
    return 0;
}