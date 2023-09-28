#include "hv/hv.h"
#include "hv/TcpServer.h"
#include "hv/hasync.h"
#include "hv/hthread.h"

#include <Windows.h>
#include <future>
#include <iostream>
#include <string>
#include <fstream>

using namespace hv;

struct HotReloadDll
{
	inline static std::string SLibName = "HotReload";
	inline static std::string SLibNameSuffix[] = {".dll", ".pdb"};

	std::string sLibName;

	HMODULE oLibHandle;

	bool LoadHandle()
	{
		if(sLibName.empty())
		{
			std::cout << "LoadHandle:: libName is NUll !" << std::endl;
			return false;
		}

		oLibHandle = LoadLibrary(sLibName.c_str());
		if(!oLibHandle)
		{
			std::cout << "LoadHandle:: cant Success!" << sLibName << std::endl;
			return false;
		}
		return true;
	};

	void FreeHandle()
	{
		if(oLibHandle)
		{
			FreeLibrary(oLibHandle);
			oLibHandle = nullptr;
		}

		if(!sLibName.empty())
		{
			for(std::string suffix : SLibNameSuffix)
			{
				std::remove((sLibName + suffix).c_str());
			}
			
			sLibName.clear();
		}
	};

	bool ReloadHandle()
	{
		FreeHandle();

		if(SLibName.empty())
		{
			std::cout << "Dll Name is Error!" << std::endl;
			return false;
		}

		std::string sufRand = std::to_string(hv_rand(10000, 99999));

		sLibName = SLibName + sufRand;

		for(std::string suffix : SLibNameSuffix)
		{
			std::ifstream dllFile(SLibName + suffix, std::ios::binary);

			if(suffix == ".pdb")
			{
				if(!dllFile)
					continue;
			}

			if(!dllFile){
				std::cout << "Dll file is not Exist!" << std::endl;
				return false;
			}

			
			std::ofstream copyDllFile(sLibName + suffix, std::ios::binary);
			if(!copyDllFile)
			{
				std::cout << "Copy Dll file is failture!" << std::endl;
				return false;
			}
			
			copyDllFile << dllFile.rdbuf();
			dllFile.close();
			copyDllFile.close();

			if(suffix == ".pdb")
			{
				if(!dllFile)
					continue;

				std::remove((SLibName + suffix).c_str());
			}	
		}

		return LoadHandle();
	};

	HotReloadDll()
	{
		memset(this, 0, sizeof(*this));
	};

	~HotReloadDll()
	{
		FreeHandle();
	};

	FARPROC GetFuncPtr(std::string funcName)
	{
		return GetProcAddress(oLibHandle, funcName.c_str());
	}

	bool OnRegServer(TcpServer* server)
	{
		if(auto funtPtr = GetFuncPtr("ServerInit"))
		{
			typedef int (*ServerInit)(TcpServer&);
			auto func = reinterpret_cast<ServerInit>(funtPtr);
			if(func)
			{
				func(*server);
				return true;
			}
		}

		return false;
	}

	bool OnUnregServer(TcpServer* server)
	{
		if(auto funtPtr = GetFuncPtr("ServerUnload"))
		{
			typedef int (*ServerUnload)(TcpServer&);
			auto func = reinterpret_cast<ServerUnload>(funtPtr);
			if(func)
			{
				func(*server);
				return true;
			}
		}

		return false;
	}
};

using CtrlHandlerType = BOOL(WINAPI *)(DWORD);

int main(int argc, char** argv)
{
	
	static HotReloadDll* PDllInfo = new HotReloadDll();
	bool res = PDllInfo->ReloadHandle();
	if(!res)
	{
		return 0;
	}

	static TcpServer* server = new TcpServer();

	if(PDllInfo->OnRegServer(server))
	{
		int listenfd = server->createsocket(555);
		if (listenfd < 0) {
			return -20;
		}
		printf("server listen on port %d, listenfd=%d ...\n", 555, listenfd);
		server->setThreadNum(4);
		server->start();
	}

	auto CtrlHandler = [](DWORD signal) -> BOOL{
		std::cout << "Programe Exit...";
		switch( signal )
		{
			case CTRL_C_EVENT:
			case CTRL_CLOSE_EVENT:
			{
				if(server)
				{
					server->stop();
					hv::async::cleanup();
					server = nullptr; 
				}

				if(PDllInfo){
					delete PDllInfo;
					PDllInfo = nullptr;
				}

				return false;
			}
		}

		return true;
	};

	SetConsoleCtrlHandler(CtrlHandler, true);

	std::string str;
    while (std::getline(std::cin, str)) {
        if (str == "quit") 
		{
			if(server)
			{
				server->stop();
				hv::async::cleanup();
				server = nullptr; 
			}
            break;
        } 
		else if (str == "reload") 
		{
			if(server)
			{
				for(int loopId = server->loop_)
				{
					loop->pause();
				}
				PDllInfo->OnUnregServer(server);
			}
			
			if(PDllInfo->ReloadHandle())
			{
				if(server)
				{
					PDllInfo->OnRegServer(server);
					while(EventLoopPtr loop = server->loop())
					{
						loop->resume();
					}
				}
			}
			
        } 
		else if (str == "pause") 
		{
			if(server)
			{
				while(EventLoopPtr loop = server->loop())
				{
					loop->pause();
				}
			}
        }
		else if (str == "resume") 
		{
			if(server)
			{
				while(EventLoopPtr loop = server->loop())
				{
					loop->resume();
				}
			}
        }
		else
		{
			std::cout << str <<std::endl;
		}
    }
	return 0;
}