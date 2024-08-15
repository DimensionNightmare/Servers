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
#include <string>
#include <iostream>
#include <future>
#include <format>
#include <filesystem>

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

enum class LunchType : uint8_t
{
	GLOBAL,
	PULL,
};

void WriteDumpFile(const char* fileName, EXCEPTION_POINTERS* ExceptionInfo = nullptr)
{
	HANDLE hDumpFile = CreateFile(
		fileName,
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
	filesystem::path execPath = argv[0];
	
#ifdef _WIN32
	SetCurrentDirectoryA(execPath.parent_path().string().c_str());
	system("chcp 65001");
#elif __unix__
	chdir(execPath.parent_path().string().c_str());
#endif

	// lunch param
	unordered_map<string, string> lunchParam = {
		{"program", execPath.filename().string()},
	};

	for (int i = 1; i < argc; i++)
	{
		string split(argv[i]);

		size_t pos = split.find('=');

		if (pos == string::npos)
		{
			DNPrint(0, LoggerLevel::Debug, "program lunch param error! Pos:%d ", i);
			return 0;
		}

		lunchParam.emplace(split.substr(0, pos), split.substr(pos + 1));
	}

	if (!lunchParam.contains("svrType"))
	{
		DNPrint(0, LoggerLevel::Error, "lunch param svrType is null! ");
		return 0;
	}

	ServerType serverType = (ServerType)stoi(lunchParam["svrType"]);
	if (serverType <= ServerType::None || serverType >= ServerType::Max)
	{
		DNPrint(0, LoggerLevel::Error, "serverType Not Invalid! ");
		return 0;
	}

	string_view serverName = EnumName(serverType);
	SetLoggerLevel(LoggerLevel::Debug, serverName);

	HV_hlog_disable();

	DNPrint(0, LoggerLevel::Normal, "hello ~");

	PInstance = make_unique<DimensionNightmare>();
	if (!PInstance->InitConfig(lunchParam))
	{
		PInstance = nullptr;
		return 0;
	}

	if (!PInstance->Init())
	{
		PInstance = nullptr;
		return 0;
	}

#ifdef _WIN32

	auto CtrlHandler = [](DWORD signal) -> BOOL
		{
			DNPrint(TipCode::TipCode_CmdOpBreak, LoggerLevel::Normal, nullptr);
			switch (signal)
			{
				case CTRL_C_EVENT:
				case CTRL_CLOSE_EVENT:
				case CTRL_SHUTDOWN_EVENT:
				case CTRL_BREAK_EVENT:
				{
					PInstance->ServerIsRun() = false;
					while (PInstance)
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
		DNPrint(ErrCode::ErrCode_CmdCtl, LoggerLevel::Error, nullptr);
		PInstance = nullptr;
		return 0;
	}

	auto UnhandledHandler = [](EXCEPTION_POINTERS* ExceptionInfo) -> long
		{
			DNPrint(TipCode::TipCode_UnhandledException, LoggerLevel::Normal, nullptr);

			string fileName = PInstance->Dll()->sDllDirRand;

			if (fileName.empty())
			{
				fileName = "MiniDump.dmp";
			}
			else
			{
				fileName = format("{}/MiniDump.dmp", fileName);
			}

			WriteDumpFile(fileName.c_str());

			PInstance->Dll()->isNormalFree = false;
			PInstance->ServerIsRun() = false;

			return EXCEPTION_CONTINUE_SEARCH;
		};

	if (!SetUnhandledExceptionFilter(UnhandledHandler))
	{
		DNPrint(ErrCode::ErrCode_UnhandledException, LoggerLevel::Error, nullptr);
		PInstance = nullptr;
		return 0;
	}
#elif __unix__

	auto CtrlHandler = [](int signal)
		{
			DNPrint(TipCode::TipCode_CmdOpBreak, LoggerLevel::Normal, nullptr);

			PInstance->ServerIsRun() = false;
		};
	signal(SIGINT, CtrlHandler);

	auto UnhandledHandler = [](int signum, siginfo_t* info, void* context)
		{
			DNPrint(TipCode::TipCode_UnhandledException, LoggerLevel::Normal, nullptr);

			delete PInstance;
			PInstance = nullptr;
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

	DNPrint(0, LoggerLevel::Normal, "Dimension Instance addr:0x%p", PInstance.get());

	auto InputEvent = async(launch::async, []()
		{
			stringstream ss;
			string str;

			bool bRun = true;

			auto quit = [&]()
				{
					PInstance->ServerIsRun() = false;
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
					string fileName;
					ss >> fileName;
					if(!fileName.empty())
					{
						WriteDumpFile(fileName.append(".dmp").c_str());
					}
				};

			std::unordered_map<string, function<void()>> cmdMap = 
			{
				#define one(func) {#func, func}

				one(quit), one(abort), one(dump_memory),
				
				#undef one
			};

			while (bRun)
			{
				getline(cin, str);

				if (!PInstance || !PInstance->ServerIsRun())
				{
					break;
				}

				if (!str.empty())
				{
					ss.clear();
					ss.str(str);
					str.clear();
					ss >> str;

					cout << "<cmd " << str << ">\n";

					if (cmdMap.contains(str))
					{
						cmdMap[str]();
					}
					else
					{
						PInstance->ExecCommand(&str, &ss);
					}

					cout << "<cmd down>\n";
				}

			}
		});

	while (PInstance && PInstance->ServerIsRun())
	{
		PInstance->TickMainFrame();
		Sleep(1);
	}

	PInstance = nullptr;

	DNPrint(0, LoggerLevel::Normal, "bye ~");

	return 0;
}