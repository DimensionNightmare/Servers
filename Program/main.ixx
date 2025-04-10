module;
#ifdef _WIN32
	#include <windef.h>
	#include <verrsrc.h>
	#include <fileapi.h>
	#include <timezoneapi.h>
	#include <consoleapi.h>
	#include <errhandlingapi.h>
	#include <processthreadsapi.h>
	#include <minidumpapiset.h>
	#include <synchapi.h>
	#include <handleapi.h>
	#include <dbghelp.h>
	#include <WinBase.h>
	#pragma comment(lib, "dbghelp.lib")
#elif __unix__
	#include <csignal>
	#include <execinfo.h>
	#include <fcntl.h>
	#include <sys/stat.h>
#endif

#include "StdMacro.h"
export module MODULE_MAIN;

import DimensionNightmare;
import Logger;
import DNServer;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import StrUtils;

#ifdef __unix__
	#define Sleep(ms) usleep(ms*1000)
#endif

enum class EMLunchType : uint8_t
{
	GLOBAL,
	PULL,
};

void WriteDumpFile(std::filesystem::path fileName, EXCEPTION_POINTERS* ExceptionInfo = nullptr)
{
	HANDLE hDumpFile = CreateFile(
		fileName.string().c_str(),
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	if (hDumpFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION info;
		info.ThreadId = GetCurrentThreadId();
		info.ExceptionPointers = ExceptionInfo;
		info.ClientPointers = FALSE;

		MINIDUMP_TYPE dumpType = (MINIDUMP_TYPE)(
			MiniDumpWithDataSegs |
			MiniDumpWithFullMemory |
			MiniDumpWithHandleData |
			MiniDumpWithThreadInfo |
			MiniDumpWithUnloadedModules |
			MiniDumpWithFullMemoryInfo |
			MiniDumpWithProcessThreadData
			);

		MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			hDumpFile,
			dumpType, // MiniDumpNormal
			ExceptionInfo ? &info : nullptr,
			nullptr,
			nullptr
		);

		CloseHandle(hDumpFile);
	}
}

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
	std::unordered_map<std::string, std::string> lunchParam = {
		{"program", execPath.string()},
	};

	for (int i = 1; i < argc; i++)
	{
		std::string split(argv[i]);

		size_t pos = split.find('=');

		if (pos == std::string::npos)
		{
			DNPrint(ELogLevel_Debug, "program lunch param error! Pos:%d ", i);
			return 0;
		}

		lunchParam.emplace(split.substr(0, pos), split.substr(pos + 1));
	}

	if (!lunchParam.contains("svrType"))
	{
		DNPrint(ELogLevel_Error, "lunch param svrType is null! ");
		return 0;
	}

	EMServerType serverType = (EMServerType)stoi(lunchParam["svrType"]);
	if (serverType <= EMServerType::None || serverType >= EMServerType::Max)
	{
		DNPrint(ELogLevel_Error, "serverType Not Invalid! ");
		return 0;
	}

	std::string_view serverName = EnumName(serverType);
	lunchParam.emplace("svrName", serverName);

	std::shared_ptr<DimensionNightmare> app = std::make_shared<DimensionNightmare>();
	if (!app->Init(lunchParam))
	{
		app = nullptr;
		return 0;
	}

	ELogLevel logLevel = ELogLevel_Debug;

	if(lunchParam.contains("LoggerLevel") && ELogLevel_Parse_(lunchParam["LoggerLevel"], &logLevel))
	{
		
	}

	SetLoggerLevel(logLevel, execPath.parent_path() / serverName);

	// hlog_disable();

	DNPrint(ELogLevel_Normal, "hello ~");

	if (!app->InitServer())
	{
		app = nullptr;
		return 0;
	}

#ifdef _WIN32

	auto CtrlHandler = [](DWORD signal) -> BOOL
		{
			DNPrintCode(EL10nCode_CmdOpBreak);
			switch (signal)
			{
				case CTRL_C_EVENT:
				case CTRL_CLOSE_EVENT:
				case CTRL_SHUTDOWN_EVENT:
				case CTRL_BREAK_EVENT:
				{
					DimensionNightmare::PInstance->ServerIsRun() = false;
					while (DimensionNightmare::PInstance)
					{
						Sleep(20);
					}
					return true;
				}
			}

			return false;
		};

	if (!SetConsoleCtrlHandler(CtrlHandler, true))
	{
		DNPrintCode(EL10nCode_CmdCtl);
		app = nullptr;
		return 0;
	}

	auto UnhandledHandler = [](EXCEPTION_POINTERS* ExceptionInfo) -> long
		{
			DNPrintCode(EL10nCode_UnhandledException);

			WriteDumpFile(HotReloadDll::PInstance->sDllDirRand / "MiniDump.dmp");

			HotReloadDll::PInstance->isNormalFree = false;
			DimensionNightmare::PInstance->ServerIsRun() = false;

			return EXCEPTION_CONTINUE_SEARCH;
		};

	if (!SetUnhandledExceptionFilter(UnhandledHandler))
	{
		DNPrintCode(EL10nCode_UnhandledException);
		app = nullptr;
		return 0;
	}
#elif __unix__

	auto CtrlHandler = [](int signal)
		{
			DNPrintCode(EL10nCode_CmdOpBreak);

			app->ServerIsRun() = false;
		};
	signal(SIGINT, CtrlHandler);

	auto UnhandledHandler = [](int signum, siginfo_t* info, void* context)
		{
			DNPrintCode(EL10nCode_UnhandledException);

			app = nullptr;
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

	DNPrint(ELogLevel_Normal, "Dimension Instance addr:0x%p", app.get());

	auto InputEvent = std::async(std::launch::async, [&]()
		{
			std::stringstream ss;
			std::string str;

			bool bRun = true;

			auto quit = [&]()
				{
					app->ServerIsRun() = false;
					bRun = false;
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
						WriteDumpFile(HotReloadDll::PInstance->sDllDirRand / fileName);
					}
				};

			std::unordered_map<std::string, std::function<void()>> cmdMap = 
			{
				#define one(func) {#func, func}

				one(quit), one(abort), one(dump_memory),
				
				#undef one
			};

			while (bRun)
			{
				std::getline(std::cin, str);

				if (!app || !app->ServerIsRun())
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
						app->ExecCommand(&str, &ss);
					}

					std::cout << "<cmd down>\n";
				}

			}
		});

	while (app && app->ServerIsRun())
	{
		app->TickMainFrame();
		Sleep(1);
	}

	app = nullptr;

	DNPrint(ELogLevel_Normal, "bye ~");

	return 0;
}
