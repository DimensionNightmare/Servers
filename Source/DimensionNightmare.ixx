module;
#include "hv/hv.h"
#include "hv/hasync.h"
#include "hv/EventLoop.h"
#include <Windows.h>
#include <DbgHelp.h>
#include <filesystem>

export module DimensionNightmare;

import ControlServer;
import GlobalServer;

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
		if (!oLibHandle)
		{
			cout << "LoadHandle:: cant Success!" << SDllName << endl;
			return false;
		}

		return true;
	};

	void FreeHandle()
	{
		if (oLibHandle)
		{
			FreeLibrary(oLibHandle);
			oLibHandle = nullptr;
		}

		if (!sDllDirRand.empty())
		{
			filesystem::remove_all(sDllDirRand.c_str());
		}

		sDllDirRand.clear();
	};

	bool ReloadHandle()
	{
		if(!std::filesystem::exists(SDllDir))
		{
			cout << "dll menu not exist!!" <<endl;
			return false;
		}

		FreeHandle();

		if (SDllName.empty())
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

	bool Init(map<string, string> &param);

	void InitCmdHandle();

	void ExecCommand(string* cmd, stringstream* ss);

	void ShutDown();

	inline DNServer *GetServer() { return pServer; }
	inline ActorManager *GetActorManager() { return pActorManager; }

	bool OnRegHotReload();

	bool OnUnregHotReload();

private:
	HotReloadDll *pHotDll;

	DNServer *pServer;

	map<string, function<void(stringstream*)>> mCmdHandle;

	ActorManager *pActorManager;
};

export DimensionNightmare *GetDimensionNightmare();

module:private;

DimensionNightmare *GetDimensionNightmare()
{
	static DimensionNightmare *PInstance = nullptr;
	if (!PInstance)
	{
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

bool DimensionNightmare::Init(map<string, string> &param)
{
	if(!param.contains("svrType"))
	{
		cout << "lunch param svrType is null" << endl;
		return false;
	}

	ServerType serverType = (ServerType)stoi(param["svrType"]);
	
	switch (serverType)
	{
	case ServerType::ControlServer:
		pServer = new ControlServer;
		break;
	case ServerType::GlobalServer:
		pServer = new GlobalServer;
		break;
	default:
		cout << "ServerType is Not Vaild!" << endl;
		return false;
	}

	if(!pServer->Init(param))
		return false;

	pHotDll = new HotReloadDll;
	if (!pHotDll->ReloadHandle())
		return false;

	pActorManager = new ActorManager;

	if (OnRegHotReload())
	{
		pServer->Start();
	}

	InitCmdHandle();

	return true;
}

void DimensionNightmare::InitCmdHandle()
{
	auto pause = [this](stringstream*)
	{
		if (pServer)
		{
			pServer->LoopEvent([](EventLoopPtr loop)
			{ 
				loop->pause(); 
			});
		}
	};

	auto resume = [this](stringstream*)
	{
		if (pServer)
		{
			pServer->LoopEvent([](EventLoopPtr loop)
			{ 
				loop->resume(); 
			});
		}
	};

	auto reload = [&, pause, resume](stringstream *ss)
	{
		pause(nullptr);
		Sleep(500);
		OnUnregHotReload();
		pHotDll->ReloadHandle();
		OnRegHotReload();
		resume(nullptr);
	};

	mCmdHandle = {
		{"pause", pause},
		{"resume", resume},
		{"reload", reload},
	};

	if(pServer)
	{
		pServer->InitCmd(mCmdHandle);
	}
}

void DimensionNightmare::ExecCommand(string* cmd, stringstream* ss)
{
	if (mCmdHandle.find(*cmd) != mCmdHandle.end())
	{
		mCmdHandle[*cmd](ss);
	}
}

void DimensionNightmare::ShutDown()
{
	if (pServer)
	{
		pServer->Stop();
		pServer = nullptr;
	}

	if (pActorManager)
	{
		delete pActorManager;
		pActorManager = nullptr;
	}

	if (pHotDll)
	{
		delete pHotDll;
		pHotDll = nullptr;
	}
}

bool DimensionNightmare::OnRegHotReload()
{
	if (auto funtPtr = pHotDll->GetFuncPtr("InitHotReload"))
	{
		typedef int (*InitHotReload)(DNServer &);
		auto func = reinterpret_cast<InitHotReload>(funtPtr);
		if (func)
		{
			func(*pServer);
			return true;
		}
	}

	return false;
}

bool DimensionNightmare::OnUnregHotReload()
{
	if (auto funtPtr = pHotDll->GetFuncPtr("ShutdownHotReload"))
	{
		typedef int (*ShutdownHotReload)(DNServer &);
		auto func = reinterpret_cast<ShutdownHotReload>(funtPtr);
		if (func)
		{
			func(*pServer);
			return true;
		}
	}

	return false;
}
