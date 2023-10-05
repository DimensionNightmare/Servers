#include <iostream>
#include <string>
#include <Windows.h>

import DimensionNightmare;

using namespace std;

int main(int argc, char** argv)
{
	DimensionNightmare* dn = GetDimensionNightmare();
	dn->Init();
	
	auto CtrlHandler = [](DWORD signal) -> BOOL{
		cout << "CtrlHandler Tirrger...";
		switch( signal )
		{
			case CTRL_C_EVENT:
			case CTRL_CLOSE_EVENT:
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
    while (getline(cin, str)) {
        if (str == "quit") 
		{
			CtrlHandler(0);
            break;
        }

		dn->ExecCommand(str);
    }
	
	return 0;
}