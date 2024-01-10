#include "hv/hlog.h"

#include <Windows.h>
#include <locale>
#include <sstream>
#include <map>
#include <vector>
#include <iostream>
#include <dbghelp.h>
#include <filesystem>

import DimensionNightmare;
import AfxCommon;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr(), __FUNCTION__, ##__VA_ARGS__);

#pragma comment(lib, "dbghelp.lib")

using namespace std;

enum class LunchType
{
	GLOBAL,
	PULL,
};

int main(int argc, char** argv)
{
	hlog_disable();
	// local output
    locale::global(locale(""));

	filesystem::path execPath = argv[0];
	SetCurrentDirectory(execPath.parent_path().string().c_str());
	//lunch param
	map<string,string> lunchParam;
	{
		string split;
		vector<string> tokens;
		for (int i = 1; i < argc; i++)
		{
			tokens.clear();
			stringstream param(argv[i]);
			
			while (getline(param, split, '=')) 
			{
				tokens.push_back(split);
			}

			if(tokens.empty() || tokens.size() != 2)
			{
				DNPrintErr("program lunch param error! Pos:%d \n", i);
				return 0;
			}

			lunchParam.emplace(tokens.front(), tokens.back());
		}
	}

	DimensionNightmare* dn = GetDimensionNightmare();
	if(!dn->Init(lunchParam))
	{
		dn->ShutDown();
		return 0;
	}
	
	auto CtrlHandler = [](DWORD signal) -> BOOL {
		DNPrint("EXITTING... CtrlHandler Tirrger... \n");
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
		DNPrintErr("Cant Set SetConsoleCtrlHandler! \n");
		return 0;
	}

	auto exceptionPtr = SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* ExceptionInfo)->long {
		DNPrint("SetUnhandledExceptionFilter Tirrger! \n");
		
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

		GetDimensionNightmare()->ShutDown();

		return EXCEPTION_CONTINUE_SEARCH;
	});
	
	if(!exceptionPtr){
		DNPrintErr("Cant Set SetUnhandledExceptionFilter! \n");
		
		return 0;
	}

	stringstream ss;
	string str;
    while (true) 
	{
		getline(cin, str);
		ss.str(str);
		ss << str;
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