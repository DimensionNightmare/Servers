#include "StdAfx.h"
#include "hv/hlog.h"

#include <map>
#include <string>
#include <iostream>
#ifdef _WIN32
	#include <Windows.h>
	#include <dbghelp.h>
	#pragma comment(lib, "dbghelp.lib")
#endif

import DimensionNightmare;

using namespace std;

enum class LunchType
{
	GLOBAL,
	PULL,
};

int main(int argc, char** argv)
{
	hlog_disable();
	
	//lunch param
	map<string,string> lunchParam;
	lunchParam.emplace("luanchPath", argv[0]);
	
	for (int i = 1; i < argc; i++)
	{
		string split(argv[i]);
		
		size_t pos = split.find('=');

		if (pos == std::string::npos)
		{
			printf("program lunch param error! Pos:%d \n", i);
			return 0;
		}

		lunchParam.emplace(split.substr(0, pos), split.substr(pos + 1));
	}
	

	DimensionNightmare* dn = GetDimensionNightmare();
	if(!dn->InitConfig(lunchParam))
	{
		dn->ShutDown();
		return 0;
	}

	if(!dn->Init())
	{
		dn->ShutDown();
		return 0;
	}

#ifdef _WIN32
	
	auto CtrlHandler = [](DWORD signal) -> BOOL {
		DNPrint(6, LoggerLevel::Normal, nullptr);
		switch (signal)
		{
			case CTRL_C_EVENT:
			case CTRL_CLOSE_EVENT:
			case CTRL_SHUTDOWN_EVENT:
			{
				GetDimensionNightmare()->ShutDown();
				return false;
			}
		}

		return true;
	};

	if(!SetConsoleCtrlHandler(CtrlHandler, true))
	{
		DNPrint(10, LoggerLevel::Error, nullptr);
		return 0;
	}

	auto exceptionPtr = SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* ExceptionInfo)->long {
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

		GetDimensionNightmare()->SetDllNotNormalFree();
		GetDimensionNightmare()->ShutDown();

		return EXCEPTION_CONTINUE_SEARCH;
	});
	
	if(!exceptionPtr)
	{
		DNPrint(11, LoggerLevel::Error, nullptr);
		return 0;
	}
#endif

	stringstream ss;
	string str;
    while (true) 
	{
		getline(cin, str);
		ss.clear();
		ss.str(str);
		str.clear();
		ss >> str;
		if(str.empty())
		{
			continue;
		}
		
        if (str == "quit") 
		{
            break;
        }
		else if(str == "abort")
		{
			int a =100;
			int b = 0;
			int c = a/b;
		}
		else
		{
			dn->ExecCommand(&str, &ss);
		}
    }

	CtrlHandler(0);
	
	return 0;
}