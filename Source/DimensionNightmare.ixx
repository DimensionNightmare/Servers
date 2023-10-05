module;


#include "hv/hv.h"
#include "hv/TcpServer.h"
#include "hv/hasync.h"
#include "hv/hthread.h"

#include <Windows.h>
#include <future>
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>

import BaseServer;

export module DimensionNightmare;

using namespace hv;
using namespace std;

struct HotReloadDll
{
	inline static string SDllName = "HotReload";
	inline static string SDllDir = "Runtime";

	HMODULE oLibHandle;

	bool LoadHandle()
	{

		oLibHandle = LoadLibraryEx(SDllName.c_str(), NULL, 0);
		if(!oLibHandle)
		{
			cout << "LoadHandle:: cant Success!" << SDllName << endl;
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
	};

	bool ReloadHandle()
	{
		FreeHandle();

		if(SDllName.empty())
		{
			cout << "Dll Name is Error!" << endl;
			return false;
		}

		for (const auto& entry : filesystem::directory_iterator(SDllDir)) {
			remove(entry.path().filename());
            filesystem::copy_file(entry.path(), entry.path().filename());
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

	FARPROC GetFuncPtr(string funcName)
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

export class DimensionNightmare
{

public:
	DimensionNightmare();

	bool Init();

	void InitCmdHandle();

	void ExecCommand(string cmd);

	void ShutDown();

private:
	HotReloadDll* pHotDll;

	BaseServer* pServer;

	map<string, function<void()>> mCmdHandle;
};

export DimensionNightmare* GetDimensionNightmare(){
	static DimensionNightmare* PInstance = nullptr;
	if(!PInstance){
		PInstance = new DimensionNightmare();
	}

	return PInstance;
}

DimensionNightmare::DimensionNightmare()
{
	pHotDll = nullptr;
	pServer = nullptr;
}

bool DimensionNightmare::Init()
{
	pHotDll = new HotReloadDll();
	if(!pHotDll->ReloadHandle())
		return false;

	pServer = new BaseServer();
	if(pHotDll->OnRegServer(pServer))
	{
		int listenfd = pServer->createsocket(555);
		if (listenfd < 0) {
			cout << "createsocket error\n";
			return false;
		}
		unpack_setting_t setting;
		setting.fixed_length = 30;
		setting.mode = unpack_mode_e::UNPACK_BY_FIXED_LENGTH;
		// pServer->setUnpack(&setting);
		printf("pServer listen on port %d, listenfd=%d ...\n", 555, listenfd);
		pServer->setThreadNum(4);
		pServer->start();
	}

	InitCmdHandle();

	return true;
}

void DimensionNightmare::InitCmdHandle()
{
	auto pause = [this]()
	{
		if(pServer)
		{
			pServer->LoopEvent([](EventLoopPtr loop){
				loop->pause();
			});
		}
	};

	auto resume = [this]()
	{
		if(pServer)
		{
			pServer->LoopEvent([](EventLoopPtr loop){
				loop->resume();
			});
		}
	};

	auto reload = [&, pause, resume]()
	{
		if(pServer)
		{
			pause();
			pHotDll->OnUnregServer(pServer);
		}
			
		if(pHotDll->ReloadHandle())
		{
			if(pServer)
			{
				pHotDll->OnRegServer(pServer);
				resume();
			}
		}
	};

    mCmdHandle = {
		{"pause", pause},
		{"resume", resume},
		{"reload", reload},
	};
}

void DimensionNightmare::ExecCommand(string cmd)
{
	if(mCmdHandle.find(cmd) != mCmdHandle.end())
	{
		mCmdHandle[cmd]();
	}
}

void DimensionNightmare::ShutDown()
{
	if(pServer)
	{
		pServer->stop();
		hv::async::cleanup();
		pServer = nullptr; 
	}

	if(pHotDll){
		delete pHotDll;
		pHotDll = nullptr;
	}
}
