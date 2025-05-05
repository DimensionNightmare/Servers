module;

export module MODULE_MAIN;

import std.compat;
import DimensionNightmare;
import Logger;
import Platform;
import StrUtils;

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

	{
		SPidLogger.Init(ELogLevel_Debug, std::nullopt);

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

		/// @brief load ini config
		ServerTypeBitFlag bitFlag;
		std::unordered_map<std::string, std::unordered_map<std::string, std::string>> iniFileParam;

		auto InitIniConfig = [&]()-> bool
		{
			if (!launchParam.contains("svrType"))
			{
				SPidLogger.Record(ELogLevel_Error, "lunch param svrType is null! ");
				return false;
			}

			for(auto& serverType : StrSplit(launchParam["svrType"], ","))
			{
				bitFlag.set(stoi(serverType));
			}
	
			launchParam.erase("svrType");
	
			uint64_t bitFlagValue = bitFlag.to_ulong();
			if (bitFlagValue == 0 || bitFlagValue >= (1 << static_cast<uint8_t>(EMServerType::Max)))
			{
				SPidLogger.Record(ELogLevel_Error, "serverType Not Invalid! ");
				return false;
			}
	
	#ifndef NDEBUG
			const char* iniFilePath = "./Config/ServerDebug.ini";
	#else
			const char* iniFilePath = "./Config/Server.ini";
	#endif
	
			if(!std::filesystem::exists(iniFilePath))
			{
				SPidLogger.Record(ELogLevel_Error, "ConfigIni Not Finded!");
				return false;
			}
	
	#ifdef _WIN32
			#define MAX_SECTION_NAME 512
			char buffer[MAX_SECTION_NAME] = { 0 };
			size_t bufferSize = sizeof(buffer);
			Platform::GetPrivateProfileSectionNamesA(buffer, MAX_SECTION_NAME, iniFilePath);
			char* current = buffer;
			while (*current)
			{
				iniFileParam[current];
				current += strlen(current) + 1;
	
				// if (iniFileParam.back().find_last_of("Server") != std::string::npos && iniFileParam.back() != serverName)
				// {
				// 	iniFileParam.pop_back();
				// }
			}
	#elif __unix__
			std::unordered_map<std::string, std::list<std::string>> sectionVal;
	
			auto GetINISectionNames = [&](const char* iniFilePath)
				{
					ifstream file(iniFilePath);
					if (!file.is_open())
					{
						cerr << "Failed to open INI file: " << iniFilePath << std::endl;
						return;
					}
	
					std::string line;
					while (getline(file, line))
					{
						if (line.empty())
						{
							continue;
						}
	
						if (line[0] == '[')
						{
							size_t endPos = line.find_first_of("]");
							if (endPos != std::string::npos)
							{
								std::string mainSection = line.substr(1, endPos - 1);
								iniFileParam.emplace_back(mainSection);
							}
						}
						else if (line[0] != ';')
						{
							sectionVal[iniFileParam.back()].emplace_back(line);
						}
					}
	
					file.close();
				};
	
			GetINISectionNames(iniFilePath);
	
			auto iter = iniFileParam.begin();
			while (iter != iniFileParam.end())
			{
				if ((*iter).find("Server") != std::string::npos && *iter != serverName)
				{
					sectionVal.erase(*iter);
					iter = iniFileParam.erase(iter);
				}
				else
				{
					++iter;
				}
			}
	#endif
			auto handler = [&](std::unordered_map<std::string, std::string>& map, std::string& split)
			{
				size_t pos = split.find('=');
				if (pos != std::string::npos)
				{
					std::string key = split.substr(0, pos);
					// if (launchParam.contains(key))
					// {
					// 	return;
					// }
	
					map.emplace(key, split.substr(pos + 1));
				}
			};
			
	
			for (auto& [mainSection, sectionMap] : iniFileParam)
			{
				if (mainSection.find_last_of("Server") != std::string::npos)
				{
					sectionMap.emplace("svrName", mainSection);
				}
	
	#ifdef _WIN32
				Platform::GetPrivateProfileSectionA(mainSection.c_str(), buffer, MAX_SECTION_NAME, iniFilePath);
				char* keyValuePair = buffer;
				while (*keyValuePair)
				{
					std::string split(keyValuePair);
					keyValuePair += strlen(keyValuePair) + 1;
					handler(sectionMap, split);
				}
	#elif __unix__
				for (const std::string& keyValuePair : sectionVal[mainSection])
				{
					std::string split(keyValuePair);
					handler(split);
				}
	#endif
			}
	
			// muti server only this valid.
			if(bitFlag.count() > 1)
			{
				iniFileParam["Common"]["program"] = launchParam["program"];
			}
			else
			{
				launchParam.merge(iniFileParam["Common"]);
				iniFileParam["Common"] = std::move(launchParam);
			}
	
			return true;
		};

		if(InitIniConfig() == false)
		{
			return 0;
		}

		SPidLogger.Init(iniFileParam["Common"]);

		App = std::make_unique<DimensionNightmare>();
		
		if (!App->Init(bitFlag, std::move(iniFileParam)))
		{
			App = nullptr;
			return 0;
		}
		
		DWORD pid = Platform::GetCurrentProcessId();

		SPidLogger.Init(std::nullopt, execPath.parent_path() / std::format("PID_{}", pid));
	}

	SPidLogger.Record(ELogLevel_Normal, "hello ~");

#ifdef _WIN32

	auto CtrlHandler = [](DWORD signal) -> BOOL
		{

			switch (signal)
			{
				// CTRL_C_EVENT = 0, CTRL_BREAK_EVENT = 1, CTRL_CLOSE_EVENT = 2, CTRL_LOGOFF_EVENT = 5, CTRL_SHUTDOWN_EVENT = 6
				case 0:
				case 1:
				case 6:
					SPidLogger.Record(EL10nCode_CmdOpBreak);
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
		SPidLogger.Record(EL10nCode_CmdCtl);
		App = nullptr;
		return 0;
	}

	auto UnhandledHandler = [](_EXCEPTION_POINTERS* ExceptionInfo) -> long
		{
			SPidLogger.Record(EL10nCode_UnhandledException);

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
		SPidLogger.Record(EL10nCode_UnhandledException);
		App = nullptr;
		return 0;
	}
#elif __unix__

	auto CtrlHandler = [](int signal)
		{
			SPidLogger.Record(EL10nCode_CmdOpBreak);
			AppRun = false;
			App = nullptr;
		};
	signal(SIGINT, CtrlHandler);

	auto UnhandledHandler = [](int signum, siginfo_t* info, void* context)
		{
			SPidLogger.Record(EL10nCode_UnhandledException);

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

	SPidLogger.Record(ELogLevel_Normal, "Dimension Instance addr->(DimensionNightmare*){}", static_cast<void*>(App.get()));

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

			auto open = [&]()
				{
					std::string allStr = execPath.string() + " ";
					while (ss >> str)
					{
						allStr += str + " ";
					}

					SPidLogger.Record(ELogLevel_Normal, "{}", allStr);

#ifdef _WIN32
					Platform::PROCESS_INFORMATION pinfo = {};
					Platform::STARTUPINFOA startInfo = {};
					memset(&startInfo, 0, sizeof(startInfo));
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

	AppRun = true;
	while (AppRun && App)
	{
		App->TickMainFrame();
		Platform::Sleep(1);
	}

	Platform::Sleep(50);

	SPidLogger.Record(ELogLevel_Normal, "bye ~");

	return 0;
}
