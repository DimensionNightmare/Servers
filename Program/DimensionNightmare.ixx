module;
#ifdef _WIN32
	#include <consoleapi2.h>
	#include <libloaderapi.h>
	#include <WinBase.h>
#elif __unix__
	#include <dlfcn.h>
#endif
#include <filesystem>
#include <iostream>
#include <format>
#include <list>
#include <random>
#include <unordered_map>
#include <string>
#include <functional>

#include "StdMacro.h"
export module DimensionNightmare;

import ControlServer;
import GlobalServer;
import AuthServer;
import GateServer;
import DatabaseServer;
import LogicServer;
import StrUtils;
import I10nText;
import Logger;
import Config.Server;
import ThirdParty.PbGen;

#ifdef __unix__
#define Sleep(ms) usleep(ms*1000)
#endif

struct HotReloadDll
{
	void* LoadHandle(string_view dllPath)
	{
#ifdef _WIN32
		string fullPath = filesystem::current_path().append(dllPath).append(SDllName).string() + ".dll";
	#ifdef NDEBUG
		SetEnvironmentVariable("PATH", "./Bin;%PATH%");
	#endif

		constexpr size_t subLen = sizeof(SDllDir);
		
		string pathDeal(dllPath);
		if (size_t pos = pathDeal.find(SDllDir); pos != string::npos)
		{
			pathDeal.erase(pos - 1, subLen);
		}

		SetConsoleTitleA(pathDeal.c_str());
		void* hModule = LoadLibraryA(fullPath.c_str());
		if (!hModule)
		{
			DNPrint(ErrCode::ErrCode_DllLoad, LoggerLevel::Error, nullptr, GetLastError());
			return nullptr;
		}

#elif __unix__
		string fullPath = filesystem::current_path().append(SDllDir).string();
		fullPath = format("{}/lib{}.so", fullPath, SDllName);
		void* hModule = dlopen(fullPath.c_str(), RTLD_LAZY);
		if (!hModule)
		{
			DNPrint(0, LoggerLevel::Debug, dlerror());
			return nullptr;
		}
#endif


		return hModule;
	};

	void FreeHandle()
	{
		if (oLibHandle)
		{
#ifdef _WIN32
			FreeLibrary((HMODULE)oLibHandle);
#elif __unix__
			dlclose(oLibHandle);
#endif
			oLibHandle = nullptr;
		}

		if (isNormalFree && !sDllDirRand.empty())
		{
			try
			{
				filesystem::remove_all(sDllDirRand.c_str());
			}
			catch (const exception& e)
			{
				DNPrint(0, LoggerLevel::Debug, "filesystem:%s", e.what());
			}
		}

		sDllDirRand.clear();
	};

	bool ReloadHandle(ServerType type)
	{
		if (!filesystem::exists(SDllDir))
		{
			DNPrint(ErrCode::ErrCode_DllMenuPath, LoggerLevel::Error, nullptr);
			return false;
		}

		if (!SDllName)
		{
			DNPrint(4, LoggerLevel::Error, nullptr);
			return false;
		}
#ifdef _WIN32

		random_device rd;
		mt19937 gen(rd());
		uniform_int_distribution<int>  u(10000, 99999);

		string newDllDirRand = format("{}/{}_{}", EnumName(type), SDllDir, u(gen));
		try
		{
			filesystem::create_directories(newDllDirRand.c_str());
			static string dllPath = format("{}/{}.dll", SDllDir, SDllName);
			filesystem::copy(dllPath.c_str(), newDllDirRand.c_str(), filesystem::copy_options::recursive);
		}
		catch (const exception& e)
		{
			DNPrint(0, LoggerLevel::Debug, "%s", e.what());
			return false;
		}
#endif
		void* hModule = LoadHandle(newDllDirRand);
		if (hModule)
		{
			FreeHandle();
			oLibHandle = hModule;
			sDllDirRand = newDllDirRand;
			return true;
		}

		return false;
	};

	HotReloadDll()
	{
		isNormalFree = true;
		oLibHandle = NULL;
	};

	~HotReloadDll()
	{
		FreeHandle();
	};

	void* GetFuncPtr(const char* funcName)
	{
#ifdef _WIN32
		return (void*)GetProcAddress((HMODULE)oLibHandle, funcName);
#elif __unix__
		return dlsym(oLibHandle, funcName);
#endif
		return nullptr;
	}
public:
	inline static const char* SDllDir = "Runtime";
	inline static const char* SDllName = "HotReload";

	string sDllDirRand;

	void* oLibHandle;

	bool isNormalFree;
};

export class DimensionNightmare
{

public:
	DimensionNightmare();

	~DimensionNightmare();

	bool InitConfig(unordered_map<string, string>& param);

	bool Init();

	void InitCmdHandle();

	void ExecCommand(string* cmd, stringstream* ss);

	bool OnRegHotReload();

	bool OnUnregHotReload();

	HotReloadDll* Dll() { return pHotDll.get(); }

	bool& ServerIsRun() { return pServer->IsRun(); }

	void TickMainFrame();

	DNl10n* GetDNl10n(){return pl10n.get();}
private:
	unique_ptr<HotReloadDll> pHotDll;

	unique_ptr<DNServer> pServer;

	unique_ptr<DNl10n> pl10n;

	unordered_map<string, function<void(stringstream*)>> mCmdHandle;

	// mount param
	unordered_map<string, string>* pLuanchParam;
};

export unique_ptr<DimensionNightmare> PInstance;

DimensionNightmare::DimensionNightmare()
{
	pLuanchParam = nullptr;
}

// need close main process
DimensionNightmare::~DimensionNightmare()
{
	pServer = nullptr;
	pHotDll = nullptr;
	pl10n = nullptr;
}

bool DimensionNightmare::InitConfig(unordered_map<string, string>& param)
{
	
#ifndef NDEBUG
		const char* iniFilePath = "../../../Config/ServerDebug.ini";
#else
		const char* iniFilePath = "./Config/Server.ini";
#endif

	if(!filesystem::exists(iniFilePath))
	{
		DNPrint(0, LoggerLevel::Error, "ConfigIni Not Finded!");
		return false;
	}

		// get ini Config
	ServerType serverType = (ServerType)stoi(param["svrType"]);
	string_view serverName = EnumName(serverType);

	vector<string> sectionNames;

#ifdef _WIN32
	char buffer[512] = { 0 };
	size_t bufferSize = sizeof(buffer);
	GetPrivateProfileSectionNamesA(buffer, bufferSize, iniFilePath);
	char* current = buffer;
	while (*current)
	{
		sectionNames.emplace_back(current);
		current += strlen(current) + 1;

		if (sectionNames.back().find_last_of("Server") != string::npos && sectionNames.back() != serverName)
		{
			sectionNames.pop_back();
		}
	}
#elif __unix__
	unordered_map<string, list<string>> sectionVal;

	auto GetINISectionNames = [&](const char* iniFilePath)
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
						sectionNames.emplace_back(sectionName);
					}
				}
				else if (line[0] != ';')
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

	for (const string& sectionName : sectionNames)
	{
#ifdef _WIN32
		GetPrivateProfileSectionA(sectionName.c_str(), buffer, bufferSize, iniFilePath);
		char* keyValuePair = buffer;
		while (*keyValuePair)
		{
			string split(keyValuePair);
			keyValuePair += strlen(keyValuePair) + 1;
#elif __unix__
		for (const string& keyValuePair : sectionVal[sectionName])
		{
			string split(keyValuePair);
#endif

			size_t pos = split.find('=');
			if (pos != string::npos)
			{
				string key = split.substr(0, pos);
				if (param.contains(key))
				{
					continue;
				}

				param.emplace(key, split.substr(pos + 1));
			}

		}
	}

	// set global Launch config  
	{
		pLuanchParam = &param;
		SetLuanchConfig(pLuanchParam);
	}

	// I10n Config
	pl10n = make_unique<DNl10n>();
	if (const char* codeStr = pl10n->InitConfigData())
	{
		DNPrint(0, LoggerLevel::Error, codeStr);
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
			pServer = make_unique<ControlServer>();
			break;
		case ServerType::GlobalServer:
			pServer = make_unique<GlobalServer>();
			break;
		case ServerType::AuthServer:
			pServer = make_unique<AuthServer>();
			break;
		case ServerType::GateServer:
			pServer = make_unique<GateServer>();
			break;
		case ServerType::DatabaseServer:
			pServer = make_unique<DatabaseServer>();
			break;
		case ServerType::LogicServer:
			pServer = make_unique<LogicServer>();
			break;
		default:
			DNPrint(ErrCode::ErrCode_SrvTypeNotVaild, LoggerLevel::Error, nullptr);
			return false;
	}

	pServer->pLuanchConfig = pLuanchParam;
	pServer->pDNl10nInstance = pl10n.get();

	if (!pServer->Init())
	{
		return false;
	}

	pHotDll = make_unique<HotReloadDll>();
	if (!pHotDll->ReloadHandle(serverType))
	{
		return false;
	}

	InitCmdHandle();

	if (!OnRegHotReload())
	{
		DNPrint(0, LoggerLevel::Error, "program lunch OnRegHotReload error!");
		return false;
	}

	if (!pServer->Start())
	{
		DNPrint(0, LoggerLevel::Error, "program lunch Server Start error!");
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

	auto reload = [&, pause, resume](stringstream* ss = nullptr)
		{
			pause();
			OnUnregHotReload();
			pHotDll->ReloadHandle(pServer->GetServerType());
			OnRegHotReload();
			resume();
		};

	auto reloadConfig = [this](stringstream* ss = nullptr)
		{
			InitConfig(*pLuanchParam);
		};

	auto open = [](stringstream* ss)
		{
			string str;
			string allStr = *GetLuanchConfigParam("program") + " ";
			while (*ss >> str)
			{
				allStr += str + " ";
			}

			cout << allStr << endl;

#ifdef _WIN32
			PROCESS_INFORMATION pinfo = {};
			STARTUPINFOA startInfo = {};
			ZeroMemory(&startInfo, sizeof(startInfo));
			startInfo.cb = sizeof(startInfo);

			startInfo.dwFlags = STARTF_USESHOWWINDOW;
			startInfo.wShowWindow = 1; // SW_SHOWNORMAL;
			if (CreateProcessA(NULL, allStr.data(),
				NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &startInfo, &pinfo))
#elif __unix__
			if (0)
#endif
			{
				cout << "success" << endl;
			}
			else
			{
				cout << "error:" << GetLastError() << endl;
			}
		};

	mCmdHandle = {
		#define one(func) {#func, func}
		
		one(pause), one(resume), one(reload), one(open),
		one(reloadConfig)
		
		#undef one
	};

	if (pServer)
	{
		pServer->InitCmd(mCmdHandle);
	}

	printf("\nCommand: ");
	for (auto& [k, v] : mCmdHandle)
	{
		printf("%s,", k.c_str());
	}

	printf("\n\n");
}

void DimensionNightmare::ExecCommand(string * cmd, stringstream * ss)
{
	if (mCmdHandle.contains(*cmd))
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
			return func(pServer.get()) == int(true);
		}
	}

	return false;
}

bool DimensionNightmare::OnUnregHotReload()
{
	// launch error pHotDll is Null
	if (!pHotDll)
	{
		return false;
	}

	if (void* funtPtr = pHotDll->GetFuncPtr("ShutdownHotReload"))
	{
		using funcSign = int (*)(DNServer*);
		if (funcSign func = reinterpret_cast<funcSign>(funtPtr))
		{
			return func(pServer.get()) == int(true);
		}
	}

	return false;
}

void DimensionNightmare::TickMainFrame()
{
	pServer->TickMainFrame();
}
