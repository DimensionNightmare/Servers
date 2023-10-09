
#include <Windows.h>
#include <locale>
#include <sstream>
#include <map>
#include <vector>
#include <iostream>
#include <dbghelp.h>

import DimensionNightmare;

using namespace std;

enum class LunchType
{
	GLOBAL,
	PULL,
};

int main(int argc, char** argv)
{
	
	// local output
    locale::global(locale(""));
	
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
				cerr << "program lunch param error! Pos: " << i << endl;
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
		cout << "CtrlHandler Tirrger...";
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
		cout << "Cant Set SetConsoleCtrlHandler!";
		return 0;
	}

	auto exceptionPtr = SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* ExceptionInfo)->long {
		cout << "SetUnhandledExceptionFilter Tirrger!";
		
		HANDLE hDumpFile = CreateFileW(
			L"MiniDump.dmp", // minidump 文件名
			GENERIC_WRITE,
			0,
			nullptr,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
		);

		if (hDumpFile != INVALID_HANDLE_VALUE) {
			MINIDUMP_EXCEPTION_INFORMATION info;
			info.ThreadId = GetCurrentThreadId();
			info.ExceptionPointers = ExceptionInfo;
			info.ClientPointers = FALSE;

			// 生成 minidump 文件
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
		cout << "Cant Set SetUnhandledExceptionFilter!";
		return 0;
	}

	string str;
    while (true) 
	{
		cin >> str;
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
			dn->ExecCommand(str);
    }

	CtrlHandler(0);
	
	return 0;
}