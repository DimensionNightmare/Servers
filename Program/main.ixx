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

enum class LunchType
{
	GLOBAL,
	PULL,
};

export int main(int argc, char **argv)
{
	hlog_disable();

	// lunch param
	map<string, string> lunchParam;
	lunchParam.emplace("luanchPath", argv[0]);

	for (int i = 1; i < argc; i++)
	{
		string split(argv[i]);

		size_t pos = split.find('=');

		if (pos == string::npos)
		{
			printf("program lunch param error! Pos:%d \n", i);
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
		DNPrint(6, LoggerLevel::Normal, nullptr);
		switch (signal)
		{
		case CTRL_C_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_SHUTDOWN_EVENT:
		{
			PInstance->ServerIsRun() = false;
			return true;
		}
		}

		return false;
	};

	if (!SetConsoleCtrlHandler(CtrlHandler, true))
	{
		DNPrint(10, LoggerLevel::Error, nullptr);
		PInstance->ShutDown();
		return 0;
	}

	auto UnhandledHandler = [](EXCEPTION_POINTERS *ExceptionInfo) -> long
	{
		DNPrint(7, LoggerLevel::Normal, nullptr);
		
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
		DNPrint(11, LoggerLevel::Error, nullptr);
		PInstance->ShutDown();
		return 0;
	}
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
				goto InputFreeze;
			}

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
InputFreeze:
			Sleep(500);
		} 
	});

	while (PInstance->ServerIsRun())
	{
		PInstance->TickMainFrame();
		Sleep(100);
	}

	PInstance->ShutDown();
	
	return 0;
}