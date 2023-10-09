
#include <Windows.h>

import DimensionNightmare;
import std.core;

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
	
	auto CtrlHandler = [](DWORD signal) -> BOOL WINAPI{
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
		cout << "Cant Set ConsoleCtrlHandler!";
		return 0;
	}

	string str;
    while (true) 
	{
		cin >> str;
        if (str == "quit") 
		{
			CtrlHandler(0);
            break;
        }

		dn->ExecCommand(str);
    }
	
	return 0;
}