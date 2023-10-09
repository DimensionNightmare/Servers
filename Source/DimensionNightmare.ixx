module;
#include "hv/hv.h"
#include "hv/hasync.h"
#include "hv/EventLoop.h"
#include <Windows.h>
#include <DbgHelp.h>
export module DimensionNightmare;

import BaseServer;
import std.core;
import std.filesystem;

#pragma comment(lib, "DbgHelp.lib")

using namespace hv;
using namespace std;

struct HotReloadDll
{
	inline static string SDllDir = "Runtime";
	inline static string SDllName = "HotReload";
	inline static string SDllNameSuffix[] = {".dll", };//".pdb"

	string sDllDirRand;

	HMODULE oLibHandle;

	bool LoadHandle()
	{
		oLibHandle = LoadLibraryEx((sDllDirRand + SDllName).c_str(), NULL, 0);
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
		for(string suffix : SDllNameSuffix)
		{
			filesystem::copy(SDllName + suffix, sDllDirRand.c_str(), filesystem::copy_options::overwrite_existing);
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

	bool OnRegServer(BaseServer* server)
	{
		if(auto funtPtr = GetFuncPtr("ServerInit"))
		{
			typedef int (*ServerInit)(BaseServer&);
			auto func = reinterpret_cast<ServerInit>(funtPtr);
			if(func)
			{
				func(*server);
				return true;
			}
		}

		return false;
	}

	bool OnUnregServer(BaseServer* server)
	{
		if(auto funtPtr = GetFuncPtr("ServerUnload"))
		{
			typedef int (*ServerUnload)(BaseServer&);
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

	bool Init(map<string,string>& param);

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

bool DimensionNightmare::Init(map<string,string>& param)
{
	/*if(!param.count("ip") || !param.count("port"))
	{
		cerr << "ip or port Need " << endl;
		return false;
	}*/

	pHotDll = new HotReloadDll();
	if(!pHotDll->ReloadHandle())
		return false;

	pServer = new BaseServer();
	/*int port = stoi(param["port"]);
	int listenfd = pServer->createsocket(port, param["ip"].c_str());*/
	int port = 555;
	int listenfd = pServer->createsocket(port);
	if (listenfd < 0) {
		cout << "createsocket error\n";
		return false;
	}
	
	printf("pServer listen on port %d, listenfd=%d ...\n", port, listenfd);

	unpack_setting_t setting;
	setting.mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
	setting.length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
	setting.body_offset = 4;
	setting.length_field_bytes = 1;
	setting.length_field_offset = 0;
	pServer->setUnpack(&setting);
	pServer->setThreadNum(4);

	if(pHotDll->OnRegServer(pServer))
	{
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

	auto loadPdb = [this]()
	{
		if (SymInitialize(GetCurrentProcess(), nullptr, TRUE)) {

			string pdbfile = pHotDll->sDllDirRand + "HotReload.pdb";

			int filesize = filesystem::file_size(pdbfile);
			DWORD64 baseAddress = SymLoadModuleEx(GetCurrentProcess(), nullptr, pdbfile.c_str(), nullptr, 0, filesize, nullptr, 0);

			if (baseAddress == 0) {
				DWORD error = GetLastError();
				std::cerr << "rttot" << error << std::endl;
			}

			SymCleanup(GetCurrentProcess());
		}
	};

    mCmdHandle = {
		{"pause", pause},
		{"resume", resume},
		{"reload", reload},
		{"loadPdb", loadPdb},
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
