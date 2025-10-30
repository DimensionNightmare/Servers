export module MAIN;

import DimensionNightmare;
import ThirdParty.Platform;
import std.compat;
import Logger;
import UniversalMemoryPool;
import ECSW;
import HotReload;

enum class EMLaunchType : uint8_t
{
	None = 0,
	GLOBAL,
	PULL,
};

bool AppRun = false;
DimensionNightmare::Ptr App;
std::filesystem::path PidFolderPath;

void CloseApp()
{
	AppRun = false;
	// App->Dispose();
	// App = nullptr;
}

#define TIMERSTART(tag) auto tag##_start = std::chrono::system_clock::now(),tag##_end = tag##_start
#define TIMEREND(tag) tag##_end = std::chrono::system_clock::now()
#define DURATION_s(tag)  printf("%s costs %I64d s\n",#tag,std::chrono::duration_cast<std::chrono::seconds>(tag##_end - tag##_start).count())
#define DURATION_ms(tag) printf("%s costs %I64d ms\n",#tag,std::chrono::duration_cast<std::chrono::milliseconds>(tag##_end - tag##_start).count());
#define DURATION_us(tag) printf("%s costs %I64d us\n",#tag,std::chrono::duration_cast<std::chrono::microseconds>(tag##_end - tag##_start).count());
#define DURATION_ns(tag) printf("%s costs %I64d ns\n",#tag,std::chrono::duration_cast<std::chrono::nanoseconds>(tag##_end - tag##_start).count());

void InputThread();

export int main(int argc, char** argv)
{

#ifdef _WIN32
	system("chcp 65001");
	Platform::SetDebugFlag();
	// 	SetCurrentDirectoryA(execPath.parent_path().string().c_str());
	// #elif __unix__
	// 	chdir(execPath.parent_path().string().c_str());
#endif

	// dynamic initializer
	{
		P_InstanceHolder = std::make_shared<InstanceHolder>();

#if 0
		int count = 1000'0000;
		int threads = 8;

		std::vector<std::future<void>> ones;

		for (int i = 0; i < threads; i++)
		{
			ones.push_back(std::async(std::launch::async, [=]()
				{
					TIMERSTART(MemPoolAlloc);

					try
					{
						std::random_device rd;
						std::mt19937 gen(rd());
						std::uniform_int_distribution<size_t> dis(0, 99999999999);

						for (int j = 0; j < count; j++)
						{
							auto a = P_InstanceHolder->GetMemPool().Allocate<DimensionNightmare>();
							// auto a = new DimensionNightmare();
							// auto ramdon = dis(gen);
							a->Dispose();

							// delete a;
						}
					}
					catch (const std::exception& e)
					{
						printf("MemPoolAlloc exception: %s\n", e.what());
					}


					TIMEREND(MemPoolAlloc);
					DURATION_ms(MemPoolAlloc);

				}));
		}

		std::vector<std::future<void>> twos;

		for (int i = 0; i < threads; i++)
		{
			twos.push_back(std::async(std::launch::async, [=]()
				{
					TIMERSTART(SysMemAlloc);

					try
					{
						std::random_device rd;
						std::mt19937 gen(rd());
						std::uniform_int_distribution<size_t> dis(0, 99999999999);

						for (int j = 0; j < count; j++)
						{
							auto a = std::make_shared<DimensionNightmare>();
							// auto ramdon = dis(gen);
							a->Dispose();
						}
					}
					catch (const std::exception& e)
					{
						printf("SysMemAlloc exception: %s\n", e.what());
					}

					TIMEREND(SysMemAlloc);
					DURATION_ms(SysMemAlloc);
				}));
		}

		for (auto& one : ones)
		{
			one.get();
		}
		for (auto& two : twos)
		{
			two.get();
		}

		// P_InstanceHolder->GetMemPool().PrintLockStats(threads);

		return 0;
# endif

	}

	ProgramConfig programConfig;

	for (int i = 1; i < argc; i++)
	{
		std::string split(argv[i]);

		size_t pos = split.find('=');

		if (pos == std::string::npos)
		{
			LoggerPrint::Log(nullptr, ELogLevel_Debug, "program lunch param error! Pos:{} ", i);
			goto POINT_EXIT;
		}

		programConfig.launchConfig.emplace(split.substr(0, pos), split.substr(pos + 1));
	}

	if (!InitProgramConfig(programConfig))
	{
		goto POINT_EXIT;
	}

	// path init. binary,log,dll...
	{

		std::filesystem::path path = argv[0];
		programConfig.iniFileConfig["Common"].emplace("ProgramDir", path.string());

		path = path.parent_path();
		programConfig.iniFileConfig["Common"].emplace("workDir", path.string());

		path = path / "Runtime/Logs";
		programConfig.iniFileConfig["Common"].emplace("LogFolder", path.string());

		path /= std::format("PID_{}", Platform::GetCurrentProcessId());
		programConfig.iniFileConfig["Common"].emplace("pidLogFolder", path.string());

		PidFolderPath = path;
	}

#ifdef _WIN32

	auto CtrlHandler = [](unsigned long signal) -> int
		{

			LoggerPrint::Log(nullptr, EL10nCode_CmdOpBreak);
			switch (signal)
			{
				// ctrl+c				ctrl+break/pause		close window			logoff					shutdown
				// CTRL_C_EVENT = 0,	CTRL_BREAK_EVENT = 1,	CTRL_CLOSE_EVENT = 2,	CTRL_LOGOFF_EVENT = 5,	CTRL_SHUTDOWN_EVENT = 6
				case 0:
					InputThread();
					return true;
				case 1:
				case 6:
					while (App && !App->HasFlag(EMProgramFlag::ResourceLoadDown))
					{
						// wait resource load down
						Platform::Sleep(20);
					}
					CloseApp();
					return true;
				case 2:
					CloseApp();
					Platform::Sleep(200);
					return true;
			}

			return false;
		};

	if (!Platform::SetConsoleCtrlHandler(CtrlHandler, true))
	{
		LoggerPrint::Log(nullptr, EL10nCode_CmdCtl);
		goto POINT_EXIT;
	}

	auto UnhandledHandler = [](_EXCEPTION_POINTERS* ExceptionInfo) -> long
		{
			// LoggerPrint::Log(nullptr, ELogLevel_Error, "Unhandled Exception! info {}",
			// 	Platform::GetStackTrace(8));

			WriteDumpFile(PidFolderPath / "MiniDump.dmp", ExceptionInfo);

			P_InstanceHolder->AuthWorld->GetSystem<HotReload>(EMSystemType::HotReload)->SetExcptionState();

			CloseApp();

			// return 0; // EXCEPTION_CONTINUE_SEARCH
			return 1; // EXCEPTION_EXECUTE_HANDLER
			// return -1; // EXCEPTION_CONTINUE_EXECUTION
		};

	if (!Platform::SetUnhandledExceptionFilter(UnhandledHandler))
	{
		LoggerPrint::Log(nullptr, EL10nCode_UnhandledException);
		goto POINT_EXIT;
	}
#elif __unix__

	auto CtrlHandler = [](int signal)
		{
			LoggerPrint::Log(nullptr, EL10nCode_CmdOpBreak);
			CloseApp();
		};
	signal(SIGINT, CtrlHandler);

	auto UnhandledHandler = [](int signum, siginfo_t* info, void* context)
		{
			LoggerPrint::Log(nullptr, EL10nCode_UnhandledException);

			CloseApp();
			exit(signum);
		};

	struct sigaction sa {};
	sa.sa_sigaction = UnhandledHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);

#endif

	P_InstanceHolder->MainWorld = App = P_InstanceHolder->GetMemPool().Allocate<DimensionNightmare>();
	if (!App->Init(programConfig))
	{
		CloseApp();
		goto POINT_EXIT;
	}

	App->StartWorlds();

	LoggerPrint::Log(nullptr, ELogLevel_Normal, "hello ~ Program Instance addr->(InstanceHolder*){:p}", static_cast<void*>(P_InstanceHolder.get()));

	AppRun = true;

	App->SetFlag(EMProgramFlag::ResourceLoadDown);

	while (AppRun && App)
	{
		App->TickMainFrame();
		Platform::Sleep(1);
	}

	Platform::Sleep(50);

POINT_EXIT:

	App = nullptr;
	P_InstanceHolder->Unload();

	LoggerPrint::Log(nullptr, ELogLevel_Normal, "bye ~");
	
	P_InstanceHolder = nullptr;

	return 0;
}

void InputThread()
{
	static std::stringstream ss;
	static std::string str;

	auto quit = []()
		{
			CloseApp();
		};

	auto abort = []()
		{
			// int a = 100;
			// int b = 0;
			// int c = a / b;

			// int* p = nullptr;
			// *p = 10;
		};

	auto dump_memory = [&]()
		{
			std::string fileName;
			ss >> fileName;
			if (!fileName.empty())
			{
				fileName.append(".dmp");
				WriteDumpFile(PidFolderPath / fileName);
			}
		};

	auto open = []()
		{
			std::string allStr = PidFolderPath.string() + " ";
			while (ss >> str)
			{
				allStr += str + " ";
			}

			LoggerPrint::Log(nullptr, ELogLevel_Normal, "{}", allStr);

#ifdef _WIN32
			Platform::PROCESS_INFORMATION pinfo{};
			Platform::STARTUPINFOA startInfo{};
			startInfo.cb = sizeof(startInfo);


			startInfo.dwFlags = 0x00000001; // STARTF_USESHOWWINDOW 0x00000001
			startInfo.wShowWindow = 1; // SW_SHOWNORMAL 1
			if (Platform::CreateProcessA(nullptr, allStr.data(), nullptr, nullptr, 0, 0x00000010, nullptr, nullptr, &startInfo, &pinfo)) // CREATE_NEW_CONSOLE 0x00000010
#elif __unix__
			if (0)
#endif
			{
				LoggerPrint::Log(nullptr, ELogLevel_Normal, "success");
			}
			else
			{
				LoggerPrint::Log(nullptr, ELogLevel_Error, "error:{}", Platform::GetLastError());
			}
		};

	static std::unordered_map<std::string, std::function<void()>> cmdMap =
	{
		#define one(func) {#func, func}

		one(quit), one(abort), one(dump_memory), one(open),

		#undef one
	};

	std::getline(std::cin, str);

	if(!AppRun || !App)
	{
		return;
	}

	ss.clear();
	ss.str(str);
	str.clear();
	ss >> str;

	LoggerPrint::Log(nullptr, ELogLevel_Normal, "<cmd {}>", str);

	if (cmdMap.contains(str))
	{
		cmdMap[str]();
	}
	else
	{
		App->ExecCommand(&str, &ss);
	}

	LoggerPrint::Log(nullptr, ELogLevel_Normal, "<cmd down>");
}
