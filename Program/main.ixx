module;
export module MAIN;

import DimensionNightmare;
import ThirdParty.Platform;
import std.compat;
import Logger;

enum class EMLunchType : uint8_t
{
	GLOBAL,
	PULL,
};


bool AppRun = false;
#define App DimensionNightmare::PInstance
#define CloseApp() 		\
	{ 					\
		App->Dispose(); \
		App = nullptr; 	\
	}


export int main(int argc, char** argv)
{
	std::filesystem::path execPath = argv[0];

#ifdef _WIN32
	system("chcp 65001");
	Platform::SetDebugFlag();
// 	SetCurrentDirectoryA(execPath.parent_path().string().c_str());
// #elif __unix__
// 	chdir(execPath.parent_path().string().c_str());
#endif

	
	SPidLogger.Init(ELogLevel_Debug);

	// lunch param
	std::unordered_map<std::string, std::string> launchParam = {
		{"program", execPath.string()},
	};

	for (int i = 1; i < argc; i++)
	{
		std::string split(argv[i]);

		size_t pos = split.find('=');

		if (pos == std::string::npos)
		{
			SPidLogger.Record(ELogLevel_Debug, "program lunch param error! Pos:{} ", i);
			return 0;
		}

		launchParam.emplace(split.substr(0, pos), split.substr(pos + 1));
	}

	static std::filesystem::path pidWorkPath = execPath.parent_path() / std::format("PID_LOG/PID_{}", Platform::GetCurrentProcessId());

	SPidLogger.Init(pidWorkPath);

	App = std::make_shared<DimensionNightmare>();
	
	if (!App->Init(std::move(launchParam)))
	{
		CloseApp();
		return 0;
	}
	
	SPidLogger.Record(ELogLevel_Normal, "hello ~");

#ifdef _WIN32

	auto CtrlHandler = [](DWORD signal) -> int
		{

			switch (signal)
			{
				// CTRL_C_EVENT = 0, CTRL_BREAK_EVENT = 1, CTRL_CLOSE_EVENT = 2, CTRL_LOGOFF_EVENT = 5, CTRL_SHUTDOWN_EVENT = 6
				case 0:
				case 1:
				case 6:
					SPidLogger.Record(EL10nCode_CmdOpBreak);
					CloseApp();
					AppRun = false;
					return true;
				case 2:
					while(true)
					{
						CloseApp();
						break;
					}
					AppRun = false;
					return true;
			}

			return false;
		};

	if (!Platform::SetConsoleCtrlHandler(CtrlHandler, true))
	{
		SPidLogger.Record(EL10nCode_CmdCtl);
		CloseApp();
		return 0;
	}

	auto UnhandledHandler = [](_EXCEPTION_POINTERS* ExceptionInfo) -> long
		{
			SPidLogger.Record(EL10nCode_UnhandledException);

			WriteDumpFile(pidWorkPath / "MiniDump.dmp", ExceptionInfo);

			CloseApp();
			AppRun = false;

			return 0; // EXCEPTION_CONTINUE_SEARCH
		};

	if (!Platform::SetUnhandledExceptionFilter(UnhandledHandler))
	{
		SPidLogger.Record(EL10nCode_UnhandledException);
		App = nullptr;
		return 0;
	}
#elif __unix__

	auto CtrlHandler = [](int signal)
		{
			SPidLogger.Record(EL10nCode_CmdOpBreak);
			AppRun = false;
			CloseApp();
		};
	signal(SIGINT, CtrlHandler);

	auto UnhandledHandler = [](int signum, siginfo_t* info, void* context)
		{
			SPidLogger.Record(EL10nCode_UnhandledException);

			AppRun = false;
			CloseApp();
			exit(signum);
		};

	struct sigaction sa{};
	sa.sa_sigaction = UnhandledHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);

#endif

	SPidLogger.Record(ELogLevel_Normal, "Dimension Instance addr->(DimensionNightmare*){}", static_cast<void*>(App.get()));

	App->AppStartInitThread();

	AppRun = true;

	auto InputThread = std::async(std::launch::deferred, [&]()
		{
			std::stringstream ss;
			std::string str;

			

			auto quit = [&]()
				{
					CloseApp();
					AppRun = false;
				};

			auto abort = [&]()
				{
					int a = 100;
					int b = 0;
					int c = a / b;
				};

			auto dump_memory = [&]()
				{
					std::string fileName;
					ss >> fileName;
					if(!fileName.empty())
					{
						fileName.append(".dmp");
						WriteDumpFile(pidWorkPath / fileName);
					}
				};

			auto open = [&]()
				{
					std::string allStr = execPath.string() + " ";
					while (ss >> str)
					{
						allStr += str + " ";
					}

					SPidLogger.Record(ELogLevel_Normal, "{}", allStr);

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
						SPidLogger.Record(ELogLevel_Normal, "success");
					}
					else
					{
						SPidLogger.Record(ELogLevel_Error, "error:{}", Platform::GetLastError());
					}
				};

			std::unordered_map<std::string, std::function<void()>> cmdMap = 
			{
				#define one(func) {#func, func}

				one(quit), one(abort), one(dump_memory), one(open)
				
				#undef one
			};

			while (AppRun)
			{
				std::getline(std::cin, str);

				if (!App)
				{
					break;
				}

				if (!str.empty())
				{
					ss.clear();
					ss.str(str);
					str.clear();
					ss >> str;

					SPidLogger.Record(ELogLevel_Normal, "<cmd {}>", str);

					if (cmdMap.contains(str))
					{
						cmdMap[str]();
					}
					else
					{
						App->ExecCommand(&str, &ss);
					}

					SPidLogger.Record(ELogLevel_Normal, "<cmd down>");
				}

			}
		});

	InputThread.get();

	while (AppRun && App)
	{
		App->TickMainFrame();
		Platform::Sleep(1);
	}

	Platform::Sleep(50);

	SPidLogger.Record(ELogLevel_Normal, "bye ~");

	return 0;
}
