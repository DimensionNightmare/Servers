// #include <Windows.h>
#include "hv/TcpClient.h"
#include "hv/hloop.h" 
#include "hv/EventLoop.h"
// #include "hv/requests.h"
// #include "hv/json.hpp"
// #include "pqxx/pqxx"
// #include "sw/redis++/redis++.h"

// #undef REPEATED
#include "google/protobuf/util/json_util.h"

// #include "GCfg/GCfg.pb.h"
// #include "GDef/GDef.pb.h"
#include "Common/Common.pb.h"

#if 0
import std.compat;

#else
#include <format>
#include <chrono>
#include <source_location>
#include <iostream>
#include <fstream>
#include <set>
#endif


using namespace hv;
using namespace std;
// using namespace sw::redis;
// using namespace GDb;
using namespace google::protobuf;

#define TIMERSTART(tag) auto tag##_start = chrono::system_clock::now(),tag##_end = tag##_start
#define TIMEREND(tag) tag##_end = chrono::system_clock::now()
#define DURATION_s(tag)  printf("%s costs %I64d s\n",#tag,chrono::duration_cast<chrono::seconds>(tag##_end - tag##_start).count())
#define DURATION_ms(tag) printf("%s costs %I64d ms\n",#tag,chrono::duration_cast<chrono::milliseconds>(tag##_end - tag##_start).count());
#define DURATION_us(tag) printf("%s costs %I64d us\n",#tag,chrono::duration_cast<chrono::microseconds>(tag##_end - tag##_start).count());
#define DURATION_ns(tag) printf("%s costs %I64d ns\n",#tag,chrono::duration_cast<chrono::nanoseconds>(tag##_end - tag##_start).count());


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
				std::cout << "key" << one.first << std::endl;
				std::cout << "value" << one.second.DebugString() << std::endl;
			}
			auto find = map.find(55);
			std::cout << "success" << std::endl;
		}
		else
		{
			std::cout << "error" << std::endl;
		}

	}

	random_device rd;
	mt19937 gen(rd());
	bernoulli_distribution  u;
	for (int i = 0; i < 5; i++)
		std::cout << u(gen) << std::endl;

	std::string msgName = GCfg::CharacterPlayer::GetDescriptor()->full_name();
	std::cout << msgName.size() << " " << msgName.length() << " " << strlen(msgName.c_str()) << std::endl;
	auto hashres = std::hash<std::string>::_Do_hash.operator()("");
	std::cout << size_t(hashres) << " " << hashres << std::endl;

	A a;
	B* b = (B*)&a;
	b->msg();
	B c;
	// c.msg();
	A* d = &c;
	d->msg();
	std::cout << sizeof(a) << std::endl;

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
				std::cout << "success" << std::endl;
			}
			else
			{
				std::cout << "error" << std::endl;
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
		const std::string& name = info->name();
		// name.empty();
		// info->set_type(GCfg::NTypeCharacterPlayer_Normal);
	}
}
#endif

#if 0
void printTime()
{
	using namespace std::chrono;
	static zoned_time<duration<long long, ratio<1, 10'000'000>>> currentZone(current_zone());
    // currentZone = system_clock::now(); 
	currentZone = system_clock::now();
	std::cout << std::format("{:%Y-%m-%d %H:%M:%S}", currentZone) << std::endl;
}


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
		wcout << "Standard Name: " << timeZoneInfo.StandardName << std::endl;
		wcout << "Daylight Name: " << timeZoneInfo.DaylightName << std::endl;
	}
	else
	{
		cerr << "Failed to get time zone information." << std::endl;
	}
#endif

	return chrono::hours(minutes / 60);
}

int main()
{
	std::string jsonstr = R"(
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
	// cout << std::format("{:%Y-%m-%d %H:%M:%S}", time_.now()) << std::endl;

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
	// wcout <<  msg << std::endl;

	printTime();

	printTime();


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
		sw::redis::Redis* con = new sw::redis::Redis(connection_options, pool_options);
		con->ping();
		std::unordered_map<std::string, std::string> hashTerm;
		con->hgetall("*", inserter(hashTerm, hashTerm.end()));

		for (const auto& [k, v] : hashTerm)
		{
			std::cout << "m[" << k << "] = (" << v << ") " << std::endl;
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	std::ios_base::sync_with_stdio(false);

	std::cin.tie(nullptr);
	std::cout.tie(nullptr);

	// std::cin.sync_with_stdio(false);

	while (true)
	{
		if (std::cin.peek() != EOF)
		{
			std::string userInput;
			std::cin >> userInput;
			std::cout << "You entered: " << userInput << std::endl;

			std::cin.ignore();
		}

		std::cout << "Doing something else..." << std::endl;
	}
}
#endif

#if 0
void BytesToHexString(std::string& bytes)
{
	std::ostringstream oss;
	oss << std::hex << std::setfill('0');
	for (unsigned char byte : bytes)
	{
		oss << std::setw(2) << static_cast<int>(byte);
	}
	bytes = oss.str();
}

void HexStringToBytes(std::string& hexString)
{
	std::string byteString = hexString;
	hexString.clear();
	for (size_t i = 0; i < byteString.length(); i += 2)
	{
		hexString += static_cast<unsigned char>(std::stoi(byteString.substr(i, 2), nullptr, 16));
	}
}


int main()
{
	GDb::Player player;
	player.set_accountid(11);
	auto propertyEntity = player.mutable_property_entity();
	propertyEntity->set_model_id(1);
	std::string msgData;
	msgData = "asdasda";
	player.SerializeToString(&msgData);

	BytesToHexString(msgData);

	std::cout << "Hexstd::string: " << msgData << std::endl;

	HexStringToBytes(msgData);

	std::cout << "Bytes: " << msgData << std::endl;
	player.Clear();
	player.ParseFromString(msgData);
	std::string msgData1;
	util::MessageToJsonString(player, &msgData1);

	std::cout << "Serlize: " << msgData1 << std::endl;

	player.Clear();
	util::JsonStringToMessage(msgData1, &player);

	std::cout << "id: " << player.accountid() << std::endl;
}
#endif

#if 0
template <typename T>
struct Task
{
	struct promise_type;
	using HandleType = std::coroutine_handle<promise_type>;
	struct promise_type
	{
		promise_type()
		{
		}

		Task get_return_object()
		{
			return Task{ HandleType::from_promise(*this) };
		}

		void return_value(const T& value)
		{
			oResult = &value;
			bReturned = true;
		}

		std::suspend_always initial_suspend() { return {}; }

		std::suspend_always final_suspend() noexcept
		{
			// Task don't Call by self, need Message handle Tick;
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

		std::coroutine_handle<> oAwaitHandle = nullptr;

		bool bReturned = false;
	};

	// Awaitable
	bool await_ready() const noexcept
	{
		return tHandle.promise().bReturned;
	}

	void await_suspend(std::coroutine_handle<> caller)
	{
		tHandle.promise().oAwaitHandle = caller;

	
	}

	void await_resume() noexcept
	{
	}
	// Awaitable

	Task(HandleType handle)
	{
		tHandle = handle;
		// SetFlag(EMTaskFlag::TimeCost);
	}

	~Task()
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

struct TaskVoid
{
	struct promise_type;
	using HandleType = std::coroutine_handle<promise_type>;
	struct promise_type
	{
		promise_type() {}

		void return_void() { bReturned = true; }

		TaskVoid get_return_object()
		{
			return TaskVoid{ HandleType::from_promise(*this) };
		}

		std::suspend_never initial_suspend() { return {}; }

		std::suspend_never final_suspend() noexcept
		{
			ReleaseAwaitHandle();
			return {};
		}

		void unhandled_exception() {}

		void ReleaseAwaitHandle()
		{
			if (oAwaitHandle) { oAwaitHandle.resume(); oAwaitHandle = nullptr; }
		}

		std::coroutine_handle<> oAwaitHandle = nullptr;

		bool bReturned = false;
	};

	// Awaitable Start
	bool await_ready() const noexcept
	{
		return tHandle.promise().bReturned;
	}

	void await_suspend(std::coroutine_handle<> caller)
	{
		tHandle.promise().oAwaitHandle = caller;
	}

	void await_resume() noexcept
	{
	}
	// Awaitable End

	TaskVoid(HandleType handle)
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



class TimerThread : public hv::EventLoopThread {
public:
    std::atomic<TimerID> nextTimerID;
    TimerThread() : hv::EventLoopThread() {
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

TaskVoid funcD() {
    std::cout << "1" << std::endl;
	co_return;
}

Task<int*> funcC() {
    std::cout << "1" << std::endl;
	int* a = new int();
	co_return a;
}

TaskVoid funcB() {
	auto res = funcC();
	
	loop->setTimeout(2500, [&](int64_t timeId){
		res.CallResume();
	});
	
    co_await res;

	// co_await funcD();
	
    std::cout << "2" << std::endl;
}

TaskVoid funcA() {
    co_await funcB();
	co_await funcD();
	co_await funcB();
	co_await funcD();
	co_await funcB();
    std::cout << "3" << std::endl;
}


int main() {
	loop = std::make_shared<TimerThread>();
    funcA();

    std::this_thread::sleep_for(std::chrono::seconds(20)); 

	loop->stop(true);
    return 0;
}
#endif


#if 0
class A
{
public:
	A(){}
	virtual ~A(){}
	int i;
};
 
class B:public A
{
public:
	B(){}
	virtual ~B(){}
	int j;
};
 
int main()
{
 
	B* pB = new B;
	A* pA;
 
	// printTime();
	TIMERSTART(Time);
	for(int i=0;i!=100000000;i++)
	{
		pA = static_cast<A*>(pB);
	}
	B* pBB;
	// printTime();
	TIMEREND(Time);

	DURATION_ms(Time);

	TIMERSTART(Time1);
 
	for(int i=0;i!=100000000;i++)
	{
		pBB= dynamic_cast<B*>(pA);
	}

	TIMEREND(Time1);

	DURATION_ms(Time1);
	// printTime();
 
 	std::cout << chrono::duration_cast<chrono::nanoseconds>(Time1_end - Time1_start).count() / chrono::duration_cast<chrono::nanoseconds>(Time_end - Time_start).count() ;

	auto timespan = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	std::cout << typeid(timespan).name() << std::endl;

	return 0;
}
#endif

#if 1

class LogColor {
public:
    inline static std::string RED	= "\033[31m";
    inline static std::string GREEN	= "\033[32m";
    inline static std::string YELLOW	= "\033[33m";
    inline static std::string BLUE	= "\033[34m";
    inline static std::string RESET	= "\033[0m";
};

std::string GetNowTimeStr()
{
	using namespace std::chrono;
	static zoned_time<system_clock::duration> currentZone(current_zone());
    currentZone = system_clock::now(); 
	return std::format("{:%Y-%m-%d %H:%M:%S}", currentZone);
}

struct LoggerPrint
{
	LoggerPrint(const std::source_location& location = std::source_location::current())
		:olocation(location)
    {	
		
	}

	~LoggerPrint()
	{
		if (oResult.empty())
		{
			return;
		}

		switch (oLevel)
		{
			case ELogLevel_Normal:
				std::cout << LogColor::BLUE << oResult << LogColor::RESET;
				break;
			case ELogLevel_Warning:
				std::cout << LogColor::YELLOW << oResult << LogColor::RESET;
				break;
			case ELogLevel_Error:
				std::cout << LogColor::RED << oResult << LogColor::RESET;
				break;
			case ELogLevel_Debug:
				std::cout << oResult;
				break;
			default:
				return;
		}

		if (LogFile.is_open())
		{
			LogFile << oResult;
			LogFile.flush();
		}
	}

	template <typename... Args>
    void operator()(ELogLevel level, const std::format_string<Args...>& fmt, Args&&... args)
	{
		oLevel = level;

		if (level < SLogLevel)
		{
			return;
		}

		std::string* locCache = nullptr;
		// if(LocCache.contains())
		// {

		// }

		oResult = std::format("[{}] {} -> \n\t{}\n", 
			GetNowTimeStr(), 
			olocation.function_name(),
			std::format(fmt, std::forward<Args>(args)...));
	}

	
	void operator()(ELogLevel level, const std::string& fmt)
	{
		oLevel = level;

		if (level < SLogLevel)
		{
			return;
		}

		oResult = std::format("[{}] {} -> \n\t{}\n", 
			GetNowTimeStr(), 
			olocation.function_name(), 
			fmt);
	}

	/// @brief set logger type and Log file Init 
	static void SetLoggerLevel(std::optional<ELogLevel> level = std::nullopt, std::optional<std::filesystem::path> path = std::nullopt)
	{
		if(level)
		{
			SLogLevel = *level;
		}
		
		if(!LogFile.is_open() && path)
		{
			if(!std::filesystem::exists(*path))
			{
				std::filesystem::create_directories(*path);
			}
			LogFile = std::ofstream( std::format("{}/Output.log", path.value().string()), std::ios::app);
		}
	}

protected:
	const std::source_location& olocation;
	std::string oResult;
	ELogLevel oLevel = ELogLevel_None;

protected:
	inline static std::ofstream LogFile; 
	inline static ELogLevel SLogLevel = ELogLevel_Normal;
	inline static std::unordered_map<std::string, std::string> LocCache;
};

class A
{
	public:
		A(){ std::cout << "A" << std::endl; }
		virtual ~A(){ std::cout << "~A" << std::endl; }

		void msg()
		{
			std::cout << "A MSG " << i << std::endl;
		}

		int i = 0;
};

class B
{
	public:
		B(){ std::cout << "B" << std::endl; }
		virtual ~B(){ std::cout << "~B" << std::endl; }

		void msg()
		{
			std::cout << "B MSG " << i << std::endl;
		}

		int i = 1;
};

class FinalExecute
{
public:
	FinalExecute(std::function<void()> func):mFunc(func)
	{

	}

	~FinalExecute()
	{
		mFunc();
	}

private:
	std::function<void()> mFunc;
};

void Func()
{
	A a;

	FinalExecute fe([&a](){
		a.msg();
	});

	a.i = 100;

	throw std::runtime_error("error");
}

std::weak_ptr<A> GetWA(std::weak_ptr<A> a)
{
	return a;
}

int main()
{
	// LoggerPrint::SetLoggerLevel(ELogLevel_Debug, "D:/Project/DimensionNightmare/Servers");
	// TIMERSTART(Time);
	// auto logger = LoggerPrint();
	// for(int i=0;i!=10'0000;i++)
	// {
	// 	logger(ELogLevel_Normal, "hello ~ {}", hv_rand(0, 1999999999));
	// }
	// TIMEREND(Time); 

	// std::cout << aa << std::endl;

	// DURATION_ms(Time);

	try
	{
		std::shared_ptr<A> a = std::make_shared<A>();
		std::weak_ptr<A> wa = a;
		wa = GetWA(a);
		Func();
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	
	return 0;
}

#endif
