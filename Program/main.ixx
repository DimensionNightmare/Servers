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
	#pragma comment(lib, "dbghelp.lib")
#elif __unix__
	#include <csignal>
	#include <execinfo.h>
	#include <fcntl.h>
	#include <sys/stat.h>
#endif
#include <map>
#include <string>
#include <iostream>
#include <future>
#include "hv/hlog.h"

#include "StdAfx.h"
export module MODULE_MAIN;

import DimensionNightmare;

using namespace std;

#ifdef __unix__
	#define Sleep(ms) usleep(ms*1000)
#endif

enum class LunchType
{
	GLOBAL,
	PULL,
};

export int main(int argc, char **argv)
{
	hlog_disable();

	SetLoggerLevel(LoggerLevel::Debug);

	// lunch param
	map<string, string> lunchParam;
	lunchParam.emplace("luanchPath", argv[0]);

	for (int i = 1; i < argc; i++)
	{
		string split(argv[i]);

		size_t pos = split.find('=');

		if (pos == string::npos)
		{
			DNPrint(0, LoggerLevel::Debug, "program lunch param error! Pos:%d \n", i);
			return 0;
		}

		lunchParam.emplace(split.substr(0, pos), split.substr(pos + 1));
	}

	PInstance = new DimensionNightmare;
	if (!PInstance->InitConfig(lunchParam))
	{
		PInstance->ShutDown();
		return 0;
	}

	if (!PInstance->Init())
	{
		PInstance->ShutDown();
		return 0;
	}

#ifdef _WIN32

	auto CtrlHandler = [](DWORD signal) -> BOOL
	{
		DNPrint(TipCode_CmdOpBreak, LoggerLevel::Normal, nullptr);
		switch (signal)
		{
		case CTRL_C_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_SHUTDOWN_EVENT:
		{
			PInstance->ServerIsRun() = false;
			while(PInstance)
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
		DNPrint(ErrCode_CmdCtl, LoggerLevel::Error, nullptr);
		PInstance->ShutDown();
		return 0;
	}

	auto UnhandledHandler = [](EXCEPTION_POINTERS *ExceptionInfo) -> long
	{
		DNPrint(TipCode_UnhandledException, LoggerLevel::Normal, nullptr);
		
		HANDLE hDumpFile = CreateFileW(
			L"MiniDump.dmp",
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

			MiniDumpWriteDump(
				GetCurrentProcess(),
				GetCurrentProcessId(),
				hDumpFile,
				MiniDumpNormal,
				&info,
				nullptr,
				nullptr
			);

			CloseHandle(hDumpFile);
		}

		PInstance->SetDllNotNormalFree();
		PInstance->ShutDown();

		return EXCEPTION_CONTINUE_SEARCH; 
	};

	if (!SetUnhandledExceptionFilter(UnhandledHandler))
	{
		DNPrint(ErrCode_UnhandledException, LoggerLevel::Error, nullptr);
		PInstance->ShutDown();
		return 0;
	}
#elif __unix__

	auto CtrlHandler = [](int signal)
	{
		DNPrint(TipCode_CmdOpBreak, LoggerLevel::Normal, nullptr);

		PInstance->ServerIsRun() = false;
	};
	signal(SIGINT, CtrlHandler);

	auto UnhandledHandler = [](int signal, siginfo_t *info, void *context)
	{
		// int fd = open("MiniDump.dmp", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		// if (fd != -1) 
		// {
		// 	void *buffer[1024];
		// 	int size = backtrace(buffer, 1024);
		// 	backtrace_symbols_fd(buffer, size, fd);
		// 	close(fd);
		// }
	};

	struct sigaction sa;
    sa.sa_sigaction = UnhandledHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);
#endif

	auto InputEvent = async(launch::async, []()
	{
		stringstream ss;
		string str;

		while (PInstance && PInstance->ServerIsRun())
		{
			getline(cin, str);
			if (str.empty())
			{
				cout << "<cmd null>\n";
			}
			else
			{
				ss.clear();
				ss.str(str);
				str.clear();
				ss >> str;

				cout << "<cmd " << str << ">\n";

				if (str == "quit")
				{
					PInstance->ServerIsRun() = false;
					break;
				}
				else if (str == "abort")
				{
					int a = 100;
					int b = 0;
					int c = a / b;
				}
				else
				{
					PInstance->ExecCommand(&str, &ss);
				}
			}

			Sleep(500);
		} 
	});

	while (PInstance->ServerIsRun())
	{
		PInstance->TickMainFrame();
		Sleep(100);
	}

	DimensionNightmare* temp = PInstance;
	PInstance = nullptr;

	temp->ShutDown();
	delete temp;
	
	DNPrint(0, LoggerLevel::Debug, "bye~");
	
	return 0;
}