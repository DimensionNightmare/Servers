module;

export module MODULE_MAIN;

import std.compat;
import DimensionNightmare;
import Logger;
import Platform;

enum class EMLunchType : uint8_t
{
	GLOBAL,
	PULL,
};

void WriteDumpFile(std::filesystem::path fileName, _EXCEPTION_POINTERS* ExceptionInfo = nullptr)
{
	auto hDumpFile = Platform::CreateFileA(
		fileName.string().c_str(),
		0x40000000L, // GENERIC_WRITE
		0,
		nullptr,
		2, // CREATE_ALWAYS
		0x00000080, // FILE_ATTRIBUTE_NORMAL
		nullptr
	);

	// INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
	if (hDumpFile != (void*)(int64_t*)-1)
	{
		Platform::_MINIDUMP_EXCEPTION_INFORMATION info;
		info.ThreadId = Platform::GetCurrentThreadId();
		info.ExceptionPointers = ExceptionInfo;
		info.ClientPointers = 0;

		Platform::MINIDUMP_TYPE dumpType = (Platform::MINIDUMP_TYPE)(
			MiniDumpWithDataSegs |
			MiniDumpWithFullMemory |
			MiniDumpWithHandleData |
			MiniDumpWithThreadInfo |
			MiniDumpWithUnloadedModules |
			MiniDumpWithFullMemoryInfo |
			MiniDumpWithProcessThreadData
			);

		Platform::MiniDumpWriteDump(
			Platform::GetCurrentProcess(),
			Platform::GetCurrentProcessId(),
			hDumpFile,
			dumpType, // MiniDumpNormal
			ExceptionInfo ? &info : nullptr,
			nullptr,
			nullptr
		);

		Platform::CloseHandle(hDumpFile);
	}
}


#define App DimensionNightmare::PInstance
static bool AppRun = false;

export int main(int argc, char** argv)
{
	std::filesystem::path execPath = argv[0];

#ifdef _WIN32
	system("chcp 65001");
// 	SetCurrentDirectoryA(execPath.parent_path().string().c_str());
// #elif __unix__
// 	chdir(execPath.parent_path().string().c_str());
#endif

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
			LoggerPrint::Log(ELogLevel_Debug, "program lunch param error! Pos:{} ", i);
			return 0;
		}

		launchParam.emplace(split.substr(0, pos), split.substr(pos + 1));
	}

	if (!launchParam.contains("svrType"))
	{
		LoggerPrint::Log(ELogLevel_Error, "lunch param svrType is null! ");
		return 0;
	}

	App = std::make_unique<DimensionNightmare>();
	
	if (!App->Init(std::move(launchParam)))
	{
		App = nullptr;
		return 0;
	}
	
	LoggerPrint::Log(ELogLevel_Normal, "hello ~");

#ifdef _WIN32

	auto CtrlHandler = [](DWORD signal) -> BOOL
		{

			switch (signal)
			{
				// CTRL_C_EVENT = 0, CTRL_BREAK_EVENT = 1, CTRL_CLOSE_EVENT = 2, CTRL_LOGOFF_EVENT = 5, CTRL_SHUTDOWN_EVENT = 6
				case 0:
				case 1:
				case 6:
					LoggerPrint()(EL10nCode_CmdOpBreak);
					AppRun = false;
					App = nullptr;
					return true;
				case 2:
					AppRun = false;
					while(true)
					{
						App = nullptr;
					}
					return true;
			}

			return false;
		};

	if (!Platform::SetConsoleCtrlHandler(CtrlHandler, true))
	{
		LoggerPrint()(EL10nCode_CmdCtl);
		App = nullptr;
		return 0;
	}

	auto UnhandledHandler = [](_EXCEPTION_POINTERS* ExceptionInfo) -> long
		{
			LoggerPrint()(EL10nCode_UnhandledException);

			if(HotReloadDll* hotdll = App->GetHotDll())
			{
				WriteDumpFile(hotdll->sDllDirRand / "MiniDump.dmp", ExceptionInfo);
				hotdll->isNormalFree = false;
			}

			AppRun = false;
			App = nullptr;

			return 0; // EXCEPTION_CONTINUE_SEARCH
		};

	if (!Platform::SetUnhandledExceptionFilter(UnhandledHandler))
	{
		LoggerPrint()(EL10nCode_UnhandledException);
		App = nullptr;
		return 0;
	}
#elif __unix__

	auto CtrlHandler = [](int signal)
		{
			LoggerPrint()(EL10nCode_CmdOpBreak);
			AppRun = false;
			App = nullptr;
		};
	signal(SIGINT, CtrlHandler);

	auto UnhandledHandler = [](int signum, siginfo_t* info, void* context)
		{
			LoggerPrint()(EL10nCode_UnhandledException);

			AppRun = false;
			App = nullptr;
			exit(signum);
		};

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = UnhandledHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);

#endif

	LoggerPrint::Log(ELogLevel_Normal, "Dimension Instance addr->(DimensionNightmare*){}", static_cast<void*>(App.get()));

	auto InputEvent = std::async(std::launch::async, [&]()
		{
			std::stringstream ss;
			std::string str;

			

			auto quit = [&]()
				{
					AppRun = false;
					App = nullptr;
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
						if(HotReloadDll* hotdll = App->GetHotDll())
						{
							WriteDumpFile(hotdll->sDllDirRand / fileName);
						}
					}
				};

			std::unordered_map<std::string, std::function<void()>> cmdMap = 
			{
				#define one(func) {#func, func}

				one(quit), one(abort), one(dump_memory),
				
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

					std::cout << "<cmd " << str << ">\n";

					if (cmdMap.contains(str))
					{
						cmdMap[str]();
					}
					else
					{
						App->ExecCommand(&str, &ss);
					}

					std::cout << "<cmd down>\n";
				}

			}
		});

	AppRun = true;
	while (AppRun && App)
	{
		App->TickMainFrame();
		Platform::Sleep(1);
	}

	Platform::Sleep(50);

	LoggerPrint::Log(ELogLevel_Normal, "bye ~");

	return 0;
}
