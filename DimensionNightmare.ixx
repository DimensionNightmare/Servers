
#include <Windows.h>
#include <future>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
	auto handle = LoadLibrary("DimensionNightmareLib.dll");
	std::future<INT_PTR> func;
MAIN:
	if(handle)
	{
		if(auto funcres = GetProcAddress(handle, "ServerStart"))
		{
			//funcres();
			func = std::async(std::launch::async, funcres);
			
		}
	}
	
	std::string str;
  
    while (std::getline(std::cin, str)) {
        if (str == "quit") {
            break;
        } else if (str == "start") {
				handle = LoadLibrary("DimensionNightmareLib.dll");
		   		goto MAIN;
        } else if (str == "stop") {
			if(handle)
			{
				if(auto funcres = GetProcAddress(handle, "ServerStop"))
				{
					funcres();
				}
           		FreeLibraryAndExitThread(handle, 0);
			}
		   handle = nullptr;
        } 
    }
	
	
	return 0;
}