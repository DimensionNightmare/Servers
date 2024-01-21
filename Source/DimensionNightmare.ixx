module;
#include "hv/hbase.h"
#include "hv/EventLoop.h"

#include <Windows.h>
#include <DbgHelp.h>
#include <filesystem>
export module DimensionNightmare;

import DNServer;
import ControlServer;
import GlobalServer;
import AuthServer;
import AfxCommon;
import GateServer;
import DatabaseServer;
import LogicServer;
import DNClientProxy;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);

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
		if(!ret)
		{
			DNPrintErr("cant set dll path! error code=%d! \n", GetLastError());
			return false;
		}
		oLibHandle = LoadLibraryEx((SDllName).c_str(), NULL, LOAD_LIBRARY_SEARCH_USER_DIRS);
		if (!oLibHandle)
		{
			DNPrintErr("cant Success! error code=%d! \n", GetLastError());
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
		if(!filesystem::exists(SDllDir))
		{
			DNPrintErr("dll menu not exist! \n");
			return false;
		}

		FreeHandle();

		if (SDllName.empty())
		{
			DNPrintErr("Dll Name is Error! \n");
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

	~DimensionNightmare();

	bool Init(map<string, string> &param);

	void InitCmdHandle();

	void ExecCommand(string* cmd, stringstream* ss);

	void ShutDown();

	DNServer *GetServer() { return pServer; }

	bool OnRegHotReload();

	bool OnUnregHotReload();

	bool OnRegClientReconnectFunc();

private:
	HotReloadDll *pHotDll;

	DNServer *pServer;

	map<string, function<void(stringstream*)>> mCmdHandle;
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
	mCmdHandle.clear();
}

DimensionNightmare::~DimensionNightmare()
{
	if(pServer)
	{
		pServer->LoopEvent([](hv::EventLoopPtr loop)
		{ 
			loop->pause(); 
		});

		OnUnregHotReload();
		
		delete pServer;
		pServer = nullptr;
	}

	if(pHotDll)
	{
		delete pHotDll;
		pHotDll = nullptr;
	}

	
}

bool DimensionNightmare::Init(map<string, string> &param)
{
	if(!param.contains("svrType"))
	{
		DNPrintErr("lunch param svrType is null! \n");
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
	case ServerType::AuthServer:
		pServer = new AuthServer;
		break;
	case ServerType::GateServer:
		pServer = new GateServer;
		break;
	case ServerType::DatabaseServer:
		pServer = new DatabaseServer;
		break;
	case ServerType::LogicServer:
		pServer = new LogicServer;
		break;
	default:
		DNPrintErr("ServerType is Not Vaild! \n");
		return false;
	}

	if(!pServer->Init(param))
	{
		return false;
	}

	pHotDll = new HotReloadDll;
	if (!pHotDll->ReloadHandle())
	{
		return false;
	}

	InitCmdHandle();

	if (OnRegHotReload())
	{
		pServer->SetRun(true); 
		pServer->Start();
	}

	OnRegClientReconnectFunc();
	return true;
}

void DimensionNightmare::InitCmdHandle()
{
	auto pause = [this](stringstream* = nullptr)
	{
		pServer->Pause();
	};

	auto resume = [this](stringstream* = nullptr)
	{
		pServer->Resume();
	};

	auto reload = [&, pause, resume](stringstream *ss = nullptr)
	{
		pause();
		// Sleep(500);
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

	if(pServer)
	{
		pServer->InitCmd(mCmdHandle);
	}

	DNPrint("cmds: ");
	for(auto &[k,v] : mCmdHandle)
		printf("%s,", k.c_str());
	
	printf("\n\n");
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
	delete this;
}

bool DimensionNightmare::OnRegHotReload()
{
	if (auto funtPtr = pHotDll->GetFuncPtr("InitHotReload"))
	{
		using funcSign = int (*)(DNServer &);
		auto func = reinterpret_cast<funcSign>(funtPtr);
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
	// launch error pHotDll is Null
	if(!pHotDll)
		return false;

	if (auto funtPtr = pHotDll->GetFuncPtr("ShutdownHotReload"))
	{
		using funcSign = int (*)(DNServer &);
		auto func = reinterpret_cast<funcSign>(funtPtr);
		if (func)
		{
			func(*pServer);
			return true;
		}
	}

	return false;
}

bool DimensionNightmare::OnRegClientReconnectFunc()
{
	if (auto funtPtr = pHotDll->GetFuncPtr("RegClientReconnectFunc"))
	{
		using funcSign = int (*)(std::function<void(DNClientProxy *, const string&, int)>);
		auto func = reinterpret_cast<funcSign>(funtPtr);
		if (func)
		{
			func(&HotReloadClient);
			return true;
		}
	}

	return false;
}
