module;
#include "hv/hv.h"
#include "hv/hasync.h"
#include "hv/EventLoop.h"
#include <Windows.h>
#include <DbgHelp.h>
#include <filesystem>
export module DimensionNightmare;

import BaseServer;
import ActorManager;

using namespace hv;
using namespace std;

struct HotReloadDll
{
	inline static string SDllDir = "Runtime";
	inline static string SDllName = "HotReload";

	string sDllDirRand;

	HMODULE oLibHandle;

	bool LoadHandle()
	{
		int ret = SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_USER_DIRS);
		ret = SetDllDirectory(sDllDirRand.c_str());
		oLibHandle = LoadLibraryEx((SDllName).c_str(), NULL, LOAD_LIBRARY_SEARCH_USER_DIRS);
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

		if(!sDllDirRand.empty())
		{
			filesystem::remove_all(sDllDirRand.c_str());
		}
		
		sDllDirRand.clear();
	};

	bool ReloadHandle()
	{
		FreeHandle();

		if(SDllName.empty())
		{
			cout << "Dll Name is Error!" << endl;
			return false;
		}

		sDllDirRand = SDllDir + to_string(hv_rand(10000, 99999)) + "/";
		filesystem::create_directories(sDllDirRand.c_str());
		filesystem::copy(SDllDir.c_str(), sDllDirRand.c_str(), filesystem::copy_options::overwrite_existing);

		return LoadHandle();
	};

	HotReloadDll()
	{
		memset(this, 0, sizeof *this);
	};

	~HotReloadDll()
	{
		FreeHandle();
	};

	FARPROC GetFuncPtr(string funcName)
	{
		return GetProcAddress(oLibHandle, funcName.c_str());
	}
};

export class DimensionNightmare
{

public:
	DimensionNightmare();

	bool Init(map<string,string>& param);

	void InitCmdHandle();

	void ExecCommand(string cmd);

	void ShutDown();

	BaseServer* GetServer();
	ActorManager* GetActorManager();

    bool OnRegHotReload();

    bool OnUnregHotReload();

private:
	HotReloadDll* pHotDll;

	BaseServer* pServer;

	map<string, function<void()>> mCmdHandle;

	ActorManager* pActorManager;
};

export DimensionNightmare* GetDimensionNightmare(){
	static DimensionNightmare* PInstance = nullptr;
	if(!PInstance){
		PInstance = new DimensionNightmare;
	}

	return PInstance;
}

DimensionNightmare::DimensionNightmare()
{
	pHotDll = nullptr;
	pServer = nullptr;
	pActorManager = nullptr;
}

bool DimensionNightmare::Init(map<string,string>& param)
{
	/*if(!param.count("ip") || !param.count("port"))
	{
		cerr << "ip or port Need " << endl;
		return false;
	}*/

	pServer = new BaseServer;
	/*int port = stoi(param["port"]);
	int listenfd = pServer->createsocket(port, param["ip"].c_str());*/
	int port = 555;
	int listenfd = pServer->createsocket(port);
	if (listenfd < 0) {
		cout << "createsocket error\n";
		return false;
	}
	
	printf("pServer listen on port %d, listenfd=%d ...\n", port, listenfd);

	auto setting = make_shared<unpack_setting_t>();

	setting->mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
	setting->length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
	setting->body_offset = 4;
	setting->length_field_bytes = 1;
	setting->length_field_offset = 0;
	pServer->setUnpack(setting.get());
	pServer->setThreadNum(4);

	pHotDll = new HotReloadDll;
	if(!pHotDll->ReloadHandle())
		return false;

	if(OnRegHotReload())
	{
		pServer->start();
	}

	pActorManager = new ActorManager;

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
		pause();
		OnUnregHotReload();
		pHotDll->ReloadHandle();
		OnRegHotReload();
		resume();
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

	if(pActorManager)
	{
		delete pActorManager;
		pActorManager = nullptr;
	}

	if(pHotDll){
		delete pHotDll;
		pHotDll = nullptr;
	}
}

BaseServer *DimensionNightmare::GetServer()
{
    return pServer;
}

ActorManager *DimensionNightmare::GetActorManager()
{
    return pActorManager;
}

bool DimensionNightmare::OnRegHotReload()
{
	if(auto funtPtr = pHotDll->GetFuncPtr("InitHotReload"))
	{
		typedef int (*InitHotReload)(DimensionNightmare&);
		auto func = reinterpret_cast<InitHotReload>(funtPtr);
		if(func)
		{
			func(*this);
			return true;
		}
	}

	return false;
}

bool DimensionNightmare::OnUnregHotReload()
{
	if(auto funtPtr = pHotDll->GetFuncPtr("ShutdownHotReload"))
	{
		typedef int (*ShutdownHotReload)(DimensionNightmare&);
		auto func = reinterpret_cast<ShutdownHotReload>(funtPtr);
		if(func)
		{
			func(*this);
			return true;
		}
	}

	return false;
}
