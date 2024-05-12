module;
#ifdef _WIN32
	#include <consoleapi2.h>
	#include <libloaderapi.h>
#elif __unix__
	#include <dlfcn.h>
#endif
#include <filesystem>
#include <iostream>
#include <format>
#include <list>
#include "hv/hbase.h"
#include "hv/EventLoop.h"

#include "StdAfx.h"
export module DimensionNightmare;

import ControlServer;
import GlobalServer;
import AuthServer;
import GateServer;
import DatabaseServer;
import LogicServer;
import Utils.StrUtils;
import I10nText;

using namespace std;

#ifdef __unix__
	#define Sleep(ms) usleep(ms*1000)
#endif

struct HotReloadDll
{
	inline static const char* SDllDir = "Runtime";
	inline static const char* SDllName = "HotReload";

	string sDllDirRand;
#ifdef _WIN32
	HMODULE oLibHandle;
#elif __unix__
	void* oLibHandle;
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
		SetDllDirectoryA(fullPath.c_str());

		#ifdef NDEBUG
		SetEnvironmentVariable("PATH", "./Bin;%PATH%");
		#endif

		// if(!ret)
		// {
		// 	("cant set dll path! error code=%d! ", GetLastError());
		// 	return false;
		// }
		// oLibHandle = LoadLibraryEx((SDllName).c_str(), NULL, LOAD_LIBRARY_SEARCH_USER_DIRS);
		// oLibHandle = LoadLibraryEx(SDllName.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS); //DONT_RESOLVE_DLL_REFERENCES |
		constexpr size_t subLen = sizeof(SDllDir);
		SetConsoleTitleA(sDllDirRand.substr(subLen).c_str());
		oLibHandle = LoadLibraryA(SDllName);
		if (!oLibHandle)
		{
			DNPrint(ErrCode_DllLoad, LoggerLevel::Error, nullptr, GetLastError());
			return false;
		}
		
#elif __unix__
		string fullPath = filesystem::current_path().append(sDllDirRand).string();
		fullPath = format("{}/lib{}.so", fullPath, SDllName);
		oLibHandle = dlopen(fullPath.c_str(), RTLD_LAZY);
		if (!oLibHandle)
		{
			DNPrint(0, LoggerLevel::Debug, dlerror());
			return false;
		}
#endif


		return true;
	};

	void FreeHandle()
	{
		if (oLibHandle)
		{
#ifdef _WIN32
			FreeLibrary(oLibHandle);
#elif __unix__
			dlclose(oLibHandle);
#endif
			Sleep(100);
			oLibHandle = nullptr;
		}

		if (isNormalFree && !sDllDirRand.empty())
		{
			try
			{
				filesystem::remove_all(sDllDirRand.c_str());
			}
			catch (const exception &e)
			{
				DNPrint(0, LoggerLevel::Debug, "filesystem:%s", e.what());
			}
		}

		sDllDirRand.clear();
	};

	bool ReloadHandle(ServerType type)
	{
		if(!filesystem::exists(SDllDir))
		{
			DNPrint(ErrCode_DllMenuPath, LoggerLevel::Error, nullptr);
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
		filesystem::copy(SDllDir, sDllDirRand.c_str(), filesystem::copy_options::recursive);

		return LoadHandle();
	};

	HotReloadDll()
	{
		// memset(this, 0, sizeof *this);
		isNormalFree = true;
		oLibHandle = NULL;
	};

	~HotReloadDll()
	{
		FreeHandle();
	};

	void *GetFuncPtr(string funcName)
	{
#ifdef _WIN32
		return GetProcAddress(oLibHandle, funcName.c_str());
#elif __unix__
		return dlsym(oLibHandle, funcName.c_str());
#endif
		return nullptr;
	}
};

export class DimensionNightmare
{

public:
	DimensionNightmare();

	~DimensionNightmare();

	bool InitConfig(map<string, string> &param);
	bool Init();

	void InitCmdHandle();

	void ExecCommand(string* cmd, stringstream* ss);

	bool OnRegHotReload();

	bool OnUnregHotReload();

	void SetDllNotNormalFree(){ pHotDll->isNormalFree = false; }

	bool& ServerIsRun(){return pServer->IsRun();}

	void TickMainFrame();

private:
	HotReloadDll *pHotDll;

	DNServer *pServer;

	map<string, function<void(stringstream*)>> mCmdHandle;

	// mount param
	map<string, string>* pLuanchParam;

	DNl10n* pl10n;
};

export DimensionNightmare *PInstance = nullptr;

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
		pServer->Stop();
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
		DNPrint(0, LoggerLevel::Debug, "lunch param svrType is null! ");
		return false;
	}

	ServerType serverType = (ServerType)stoi(param["svrType"]);
	if(serverType <= ServerType::None || serverType >= ServerType::Max)
	{
		DNPrint(0, LoggerLevel::Debug, "serverType Not Invalid! ");
		return false;
	}

	filesystem::path execPath = param["luanchPath"];

#ifdef _WIN32
	SetCurrentDirectoryA(execPath.parent_path().string().c_str());
#elif __unix__
	chdir(execPath.parent_path().string().c_str());
#endif

	{
		#ifndef NDEBUG
		const char *iniFilePath = "../../../Config/ServerDebug.ini";
		#else
		const char *iniFilePath = "./Config/Server.ini";
#endif

		// get ini Config
		string_view serverName = EnumName(serverType);

		vector<string> sectionNames;

#ifdef _WIN32
		char buffer[512] = {0};
		size_t bufferSize = sizeof(buffer);
		GetPrivateProfileSectionNamesA(buffer, bufferSize, iniFilePath);
		char *current = buffer;
		while (*current) 
		{
			sectionNames.push_back(current);
			current += strlen(current) + 1;

			if (sectionNames.back().find_last_of("Server") != string::npos && sectionNames.back() != serverName)
			{
				sectionNames.pop_back();
			}
		}
#elif __unix__
		map<string, list<string>> sectionVal;

		auto GetINISectionNames = [&](const char *iniFilePath)
		{
			ifstream file(iniFilePath);
			if (!file.is_open())
			{
				cerr << "Failed to open INI file: " << iniFilePath << endl;
				return;
			}

			string line;
			while (getline(file, line))
			{
				if (line.empty())
				{
					continue;
				}

				if (line[0] == '[')
				{
					size_t endPos = line.find_first_of("]");
					if (endPos != string::npos)
					{
						string sectionName = line.substr(1, endPos - 1);
						sectionNames.push_back(sectionName);
					}
				}
				else if(line[0] != ';')
				{
					sectionVal[sectionNames.back()].emplace_back(line);
				}
			}

			file.close();
		};

		GetINISectionNames(iniFilePath);

		auto iter = sectionNames.begin();
		while (iter != sectionNames.end())
		{
			if ((*iter).find("Server") != std::string::npos && *iter != serverName)
			{
				sectionVal.erase(*iter);
				iter = sectionNames.erase(iter);
			}
			else
			{
				++iter;
			}
		}
#endif

		for (const string &sectionName : sectionNames)
		{
#ifdef _WIN32
            GetPrivateProfileSectionA(sectionName.c_str(), buffer, bufferSize, iniFilePath);
			char *keyValuePair = buffer;
            while (*keyValuePair) 
			{
				string split(keyValuePair);
				keyValuePair += strlen(keyValuePair) + 1;
#elif __unix__
			for(const string& keyValuePair : sectionVal[sectionName])
			{
				string split(keyValuePair);
#endif

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
	// *** if use locale::global, webProxy cant push file to webclient, dont kown why ***
    // locale::global(locale(param["locale"]));
	// wcout.imbue(locale("zh_CN.UTF-8"));
	// wcout.imbue(locale("zh_CN.UTF-8"));
	// cout.imbue(locale("zh_CN.UTF-8"));
#ifdef _WIN32
	system("chcp 65001");
#endif
	// set global Launch config  
	{
		pLuanchParam = &param;
		SetLuanchConfig(pLuanchParam);
	}

	// I10n Config
	pl10n = new DNl10n();
	if(const char* codeStr = pl10n->InitConfigData())
	{
		DNPrint(0, LoggerLevel::Debug, codeStr);
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
		pServer = new ControlServer();
		break;
	case ServerType::GlobalServer:
		pServer = new GlobalServer();
		break;
	case ServerType::AuthServer:
		pServer = new AuthServer();
		break;
	case ServerType::GateServer:
		pServer = new GateServer();
		break;
	case ServerType::DatabaseServer:
		pServer = new DatabaseServer();
		break;
	case ServerType::LogicServer:
		pServer = new LogicServer();
		break;
	default:
		DNPrint(ErrCode_SrvTypeNotVaild, LoggerLevel::Error, nullptr);
		return false;
	}

	pServer->pLuanchConfig = pLuanchParam;
	pServer->pDNl10nInstance = pl10n;

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
	
	return pServer->IsRun() = true;
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
		PROCESS_INFORMATION pinfo = {}; 
		STARTUPINFOA startInfo  = {};
		ZeroMemory(&startInfo, sizeof(startInfo));
    	startInfo.cb = sizeof(startInfo);

		startInfo.wShowWindow = SW_NORMAL;
		startInfo.dwFlags = STARTF_USESHOWWINDOW;
		if(CreateProcessA(NULL, allStr.data(),
			NULL,NULL,FALSE,CREATE_NEW_CONSOLE,NULL,NULL, &startInfo, &pinfo))
#elif __unix__
		if (0)
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

	printf("\nCommand: ");
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

bool DimensionNightmare::OnRegHotReload()
{
	if (void* funtPtr = pHotDll->GetFuncPtr("InitHotReload"))
	{
		using funcSign = int (*)(DNServer*);
		if (funcSign func = reinterpret_cast<funcSign>(funtPtr))
		{
			return func(pServer) == int(true);
		}
	}

	return false;
}

bool DimensionNightmare::OnUnregHotReload()
{
	// launch error pHotDll is Null
	if(!pHotDll)
	{
		return false;
	}

	if (void* funtPtr = pHotDll->GetFuncPtr("ShutdownHotReload"))
	{
		using funcSign = int (*)(DNServer*);
		if (funcSign func = reinterpret_cast<funcSign>(funtPtr))
		{
			return func(pServer) == int(true);
		}
	}

	return false;
}

void DimensionNightmare::TickMainFrame()
{
	pServer->TickMainFrame();
}
