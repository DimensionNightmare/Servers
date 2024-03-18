module;
#include "StdAfx.h"
#include "hv/hbase.h"
#include "hv/EventLoop.h"

#include <filesystem>
#include <locale>
#include <iostream>
#include <format>
#ifdef _WIN32
	#include <Windows.h>
#endif
export module DimensionNightmare;

import DNServer;
import ControlServer;
import GlobalServer;
import AuthServer;
import GateServer;
import DatabaseServer;
import LogicServer;
import Utils.StrUtils;


using namespace std;

struct HotReloadDll
{
	inline static const char* SDllDir = "Runtime";
	inline static const char* SDllName = "HotReload";

	string sDllDirRand;
#ifdef _WIN32
	HMODULE oLibHandle;
#endif

	bool isNormalFree;

	bool LoadHandle()
	{
#ifdef _WIN32
		// int ret = SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_USER_DIRS);
		// ret = SetDllDirectory(sDllDirRand.c_str());
		string fullPath = filesystem::current_path().append(sDllDirRand).string();
		// wstring wstr(fullPath.begin(), fullPath.end());
		// AddDllDirectory(wstr.c_str());
		SetDllDirectory(fullPath.c_str());

		SetEnvironmentVariable("PATH", "./Bin;%PATH%");

		// if(!ret)
		// {
		// 	("cant set dll path! error code=%d! \n", GetLastError());
		// 	return false;
		// }
		// oLibHandle = LoadLibraryEx((SDllName).c_str(), NULL, LOAD_LIBRARY_SEARCH_USER_DIRS);
		// oLibHandle = LoadLibraryEx(SDllName.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS); //DONT_RESOLVE_DLL_REFERENCES |
		constexpr size_t subLen = sizeof(SDllDir);
		SetConsoleTitle(sDllDirRand.substr(subLen).c_str());
		oLibHandle = LoadLibrary(SDllName);
#endif
		if (!oLibHandle)
		{
			DNPrint(2, LoggerLevel::Error, nullptr, GetLastError());
			return false;
		}

		return true;
	};

	void FreeHandle()
	{
		if (oLibHandle)
		{
#ifdef _WIN32
			FreeLibrary(oLibHandle);
#endif
			oLibHandle = nullptr;
		}

		if (isNormalFree && !sDllDirRand.empty())
		{
			filesystem::remove_all(sDllDirRand.c_str());
		}

		sDllDirRand.clear();
	};

	bool ReloadHandle(ServerType type)
	{
		if(!filesystem::exists(SDllDir))
		{
			DNPrint(3, LoggerLevel::Error, nullptr);
			return false;
		}

		FreeHandle();

		if (!SDllName)
		{
			DNPrint( 4, LoggerLevel::Error,nullptr);
			return false;
		}

		sDllDirRand = format("{}_{}_{}", SDllDir, EnumName(type), hv_rand(10000, 99999));
		filesystem::create_directories(sDllDirRand.c_str());
		filesystem::copy(SDllDir, sDllDirRand.c_str(), filesystem::copy_options::overwrite_existing);

		return LoadHandle();
	};

	HotReloadDll()
	{
		memset(this, 0, sizeof *this);
		isNormalFree = true;
	};

	~HotReloadDll()
	{
		FreeHandle();
	};

	void* GetFuncPtr(string funcName)
	{
#ifdef _WIN32
		return GetProcAddress(oLibHandle, funcName.c_str());
#endif
	}
};

export class DimensionNightmare
{

public:
	DimensionNightmare();

	~DimensionNightmare();

	bool InitConfig(map<string, string> &param);
	bool Init();
	void InitDllEnv();

	void InitCmdHandle();

	void ExecCommand(string* cmd, stringstream* ss);

	void ShutDown();

	DNServer *GetServer() { return pServer; }

	bool OnRegHotReload();

	bool OnUnregHotReload();

	bool OnRegClientReconnectFunc();

	void SetDllNotNormalFree(){ pHotDll->isNormalFree = false; }
private:
	HotReloadDll *pHotDll;

	DNServer *pServer;

	map<string, function<void(stringstream*)>> mCmdHandle;

	// mount param
	map<string, string>* pLuanchParam;

	DNl10n* pl10n;
};

export DimensionNightmare *GetDimensionNightmare();



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
	pl10n = nullptr;
	pLuanchParam = nullptr;
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

	if(pl10n)
	{
		delete pl10n;
		pl10n = nullptr;
	}
}

bool DimensionNightmare::InitConfig(map<string, string> &param)
{
	if(!param.contains("svrType"))
	{
		printf("lunch param svrType is null! \n");
		return false;
	}

	ServerType serverType = (ServerType)stoi(param["svrType"]);
	if(serverType <= ServerType::None || serverType >= ServerType::Max)
	{
		printf("serverType Not Invalid! \n");
		return false;
	}

	filesystem::path execPath = param["luanchPath"];
	SetCurrentDirectory(execPath.parent_path().string().c_str());

	{
		#if _DEBUG
		LPCSTR iniFilePath = "../../../Config/ServerDebug.ini";
		#else
		LPCSTR iniFilePath = "./Config/Server.ini";
		#endif

		const int bufferSize = 512;
		
		char buffer[bufferSize] = {0};

		vector<string> sectionNames;

#ifdef _WIN32
		GetPrivateProfileSectionNamesA(buffer, bufferSize, iniFilePath);
#endif

		// get ini Config
		string_view serverName = EnumName(serverType);

		char* current = buffer;
		while (*current) 
		{
			sectionNames.push_back(current);
			current += strlen(current) + 1;

			if (sectionNames.back().find_last_of("Server") != string::npos && sectionNames.back() != serverName)
			{
				sectionNames.pop_back();
			}
		}

		for (const auto& sectionName : sectionNames) 
		{
            GetPrivateProfileSectionA(sectionName.c_str(), buffer, bufferSize, iniFilePath);

            CHAR* keyValuePair = buffer;
            while (*keyValuePair) 
			{
				string split(keyValuePair);
				keyValuePair += strlen(keyValuePair) + 1;

				size_t pos = split.find('=');
				if (pos != string::npos) 
				{
					string key = split.substr(0, pos);
					if(param.contains(key)) 
					{
						continue;
					}

					param.emplace(key, split.substr(pos + 1));
				}
                
            }
        }
	}

	// local output
    locale::global(locale(param["locale"]));

	// set global Launch config  
	{
		pLuanchParam = &param;
		SetLuanchConfig(pLuanchParam);
	}

	// I10n Config
	pl10n = new DNl10n();
	if(!pl10n->InitConfigData())
	{
		return false;
	}

	return true;
}

bool DimensionNightmare::Init()
{
	string* value = GetLuanchConfigParam("svrType");
	ServerType serverType = (ServerType)stoi(*value);
	
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
		DNPrint(5, LoggerLevel::Error, nullptr);
		return false;
	}

	if(!pServer->Init())
	{
		return false;
	}

	pHotDll = new HotReloadDll;
	if (!pHotDll->ReloadHandle(serverType))
	{
		return false;
	}

	InitCmdHandle();

	if (!OnRegHotReload())
	{
		return false;	
	}

	if(!pServer->Start())
	{
		return false;
	}
	
	pServer->SetRun(true); 

	OnRegClientReconnectFunc();
	return true;
}

void DimensionNightmare::InitDllEnv()
{
	SetLuanchConfig(pLuanchParam);
	SetDNl10nInstance(pl10n);
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
		pHotDll->ReloadHandle(pServer->GetServerType());
		OnRegHotReload();
		resume();
	};

	auto open = [](stringstream* ss)
	{
		string str;
		string allStr = *GetLuanchConfigParam("luanchPath") + " ";
		while(*ss >> str)
		{
			allStr += str + " ";
		}

		cout << allStr << endl;

#ifdef _WIN32
		STARTUPINFO startInfo{}; 
		PROCESS_INFORMATION pinfo{};

		startInfo.wShowWindow = SW_NORMAL;
		startInfo.dwFlags = STARTF_USESHOWWINDOW;
		if(CreateProcessA(NULL, (char*)allStr.c_str(),
			NULL,NULL,FALSE,CREATE_NEW_CONSOLE,NULL,NULL,&startInfo,&pinfo))
#endif
		{
			cout << "success" << endl;
		}
		else
		{
			cout << "error" << endl;
		}
	};

	mCmdHandle = {
		{"pause", pause},
		{"resume", resume},
		{"reload", reload},
		{"open", open},
	};

	if(pServer)
	{
		pServer->InitCmd(mCmdHandle);
	}

	printf("cmds: ");
	for(auto &[k,v] : mCmdHandle)
	{
		printf("%s,", k.c_str());
	}
	
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
	if (void* funtPtr = pHotDll->GetFuncPtr("InitHotReload"))
	{
		using funcSign = int (*)(DimensionNightmare&);
		if (funcSign func = reinterpret_cast<funcSign>(funtPtr))
		{
			return func(*this) == int(true);
		}
	}

	return false;
}

bool DimensionNightmare::OnUnregHotReload()
{
	// launch error pHotDll is Null
	if(!pHotDll)
		return false;

	if (void* funtPtr = pHotDll->GetFuncPtr("ShutdownHotReload"))
	{
		using funcSign = int (*)(DimensionNightmare&);
		if (funcSign func = reinterpret_cast<funcSign>(funtPtr))
		{
			return func(*this) == int(true);
		}
	}

	return false;
}

bool DimensionNightmare::OnRegClientReconnectFunc()
{
	if (void* funtPtr = pHotDll->GetFuncPtr("RegClientReconnectFunc"))
	{
		using funcSign = int (*)(function<void(const char*, int)>);
		if (funcSign func = reinterpret_cast<funcSign>(funtPtr))
		{
			// auto funcProxy = bind(&DNServer::ReClientEvent, pServer);
			auto funcProxy = [this](const char* ip, int port)
			{
				pServer->ReClientEvent(ip, port);
			};
			func(funcProxy);
			return true;
		}
	}

	return false;
}
