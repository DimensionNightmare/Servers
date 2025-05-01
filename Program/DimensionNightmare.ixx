module;

export module DimensionNightmare;

import ControlServer;
import GlobalServer;
import AuthServer;
import GateServer;
import DatabaseServer;
import LogicServer;
import StrUtils;
import L10nText;
import Logger;
import Config.Server;
import ThirdParty.PbGen;
import DNServer;
import DllUtils;
import std.compat;
import Platform;

export struct HotReloadDll
{
	/// @brief load dll/so runtime library
	Platform::HotHandle LoadHandle(std::filesystem::path dllPath)
	{
#ifdef _WIN32
		dllPath = dllPath.append(SDllName);
	#ifdef NDEBUG
		SetEnvironmentVariableA("PATH", "./Bin;%PATH%");
	#endif

		Platform::HotHandle hModule = Platform::LoadLibraryA(dllPath.string().c_str());
		if (!hModule)
		{
			LoggerPrint()(EL10nCode_DllLoad, Platform::GetLastError());
			return nullptr;
		}

#elif __unix__
		std::string fullPath = filesystem::current_path().append(sDllDir).string();
		fullPath = std::format("{}/lib{}.so", fullPath, SDllName);
		void* hModule = dlopen(fullPath.c_str(), RTLD_LAZY);
		if (!hModule)
		{
			LoggerPrint()(ELogLevel_Debug, dlerror());
			return nullptr;
		}
#endif


		return hModule;
	}

	/// @brief unload dll/so runtime library
	void FreeHandle()
	{
		if (oLibHandle)
		{
#ifdef _WIN32
			Platform::FreeLibrary(oLibHandle);
#elif __unix__
			dlclose(oLibHandle);
#endif
			oLibHandle = nullptr;
		}

		if (isNormalFree && !sDllDirRand.empty())
		{
			try
			{
				std::filesystem::remove_all(sDllDirRand.c_str());
			}
			catch (const std::exception& e)
			{
				LoggerPrint()(ELogLevel_Debug, "filesystem:{}", e.what());
			}
		}

		sDllDirRand.clear();
	}

	/// @brief reload dll/so runtime library
	bool ReloadHandle()
	{
		if (!std::filesystem::exists(sDllDir))
		{
			LoggerPrint()(EL10nCode_DllMenuPath);
			return false;
		}

		if (!SDllName)
		{
			LoggerPrint()(EL10nCode_DllFileName);
			return false;
		}
#ifdef _WIN32

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int>  u(10000, 99999);

		int randNum = u(gen);
		std::filesystem::path newDllDir = sDllDir.parent_path() / std::format("{}/Runtime_{}", sServerName, randNum);
		try
		{
			std::filesystem::create_directories(newDllDir);
			std::filesystem::copy(sDllDir /* / SDllName*/, newDllDir, std::filesystem::copy_options::recursive);
		}
		catch (const std::exception& e)
		{
			LoggerPrint()(ELogLevel_Debug, "{}", e.what());
			return false;
		}
#endif
		Platform::HotHandle hModule = LoadHandle(newDllDir);
		if (hModule)
		{
			FreeHandle();
			oLibHandle = hModule;
			sDllDirRand = newDllDir;
			Platform::SetConsoleTitleA(std::format("{}_{}", sServerName, randNum).c_str());
			return true;
		}

		return false;
	}

	/// @brief
	HotReloadDll()
	{
		sDllDir = std::filesystem::path(*LaunchConfig::GetParam("program")).parent_path() / sDllDir;
		sServerName = *LaunchConfig::GetParam("svrName");
	}

	/// @brief
	~HotReloadDll()
	{
		FreeHandle();
	}

	/// @brief get runtime lib funcpointer
	Platform::FuncHandle GetFuncPtr(const char* funcName)
	{
#ifdef _WIN32
		return Platform::GetProcAddress(oLibHandle, funcName);
#elif __unix__
		return dlsym(oLibHandle, funcName);
#endif
		return nullptr;
	}

public:
	/// @brief runtime library floder name
	std::filesystem::path sDllDir = "Runtime";

	/// @brief runtime library file name
	inline static const char* SDllName = "HotReload.dll";

	std::filesystem::path sDllDirRand;

	/// @brief runtime library loaded pointer
	Platform::HotHandle oLibHandle = nullptr;

	/// @brief nomal exit or exception exit
	bool isNormalFree = true;

	std::string sServerName;
};

export class DimensionNightmare
{

public:
	/// @brief
	DimensionNightmare()
	{
	}

	// need close main process
	~DimensionNightmare()
	{
		pServer = nullptr;
		pHotDll = nullptr;
		pDNl10n = nullptr;
		pLaunchConfig = nullptr;
	}

	/// @brief load ini config
	bool Init(std::unordered_map<std::string, std::string>& launchParam)
	{
		EMServerType serverType = (EMServerType)stoi(launchParam["svrType"]);
		if (serverType <= EMServerType::None || serverType >= EMServerType::Max)
		{
			LoggerPrint()(ELogLevel_Error, "serverType Not Invalid! ");
			return false;
		}

		std::string_view serverName = EnumName(serverType);
		launchParam.emplace("svrName", serverName);

#ifndef NDEBUG
		const char* iniFilePath = "./Config/ServerDebug.ini";
#else
		const char* iniFilePath = "./Config/Server.ini";
#endif

		if(!std::filesystem::exists(iniFilePath))
		{
			LoggerPrint()(ELogLevel_Error, "ConfigIni Not Finded!");
			return false;
		}

		// get ini Config

		std::vector<std::string> sectionNames;

#ifdef _WIN32
		#define MAX_SECTION_NAME 512
		char buffer[MAX_SECTION_NAME] = { 0 };
		size_t bufferSize = sizeof(buffer);
		Platform::GetPrivateProfileSectionNamesA(buffer, MAX_SECTION_NAME, iniFilePath);
		char* current = buffer;
		while (*current)
		{
			sectionNames.emplace_back(current);
			current += strlen(current) + 1;

			if (sectionNames.back().find_last_of("Server") != std::string::npos && sectionNames.back() != serverName)
			{
				sectionNames.pop_back();
			}
		}
#elif __unix__
		std::unordered_map<std::string, std::list<std::string>> sectionVal;

		auto GetINISectionNames = [&](const char* iniFilePath)
			{
				ifstream file(iniFilePath);
				if (!file.is_open())
				{
					cerr << "Failed to open INI file: " << iniFilePath << std::endl;
					return;
				}

				std::string line;
				while (getline(file, line))
				{
					if (line.empty())
					{
						continue;
					}

					if (line[0] == '[')
					{
						size_t endPos = line.find_first_of("]");
						if (endPos != std::string::npos)
						{
							std::string sectionName = line.substr(1, endPos - 1);
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
		auto handler = [&](std::string& split)
		{
			size_t pos = split.find('=');
			if (pos != std::string::npos)
			{
				std::string key = split.substr(0, pos);
				if (launchParam.contains(key))
				{
					return;
				}

				launchParam.emplace(key, split.substr(pos + 1));
			}
		};
		

		for (const std::string& sectionName : sectionNames)
		{
#ifdef _WIN32
			Platform::GetPrivateProfileSectionA(sectionName.c_str(), buffer, MAX_SECTION_NAME, iniFilePath);
			char* keyValuePair = buffer;
			while (*keyValuePair)
			{
				std::string split(keyValuePair);
				keyValuePair += strlen(keyValuePair) + 1;
				handler(split);
			}
#elif __unix__
			for (const std::string& keyValuePair : sectionVal[sectionName])
			{
				std::string split(keyValuePair);
				handler(split);
			}
#endif
		}

		// set global Launch config  
		pLaunchConfig = std::make_shared<LaunchConfig>();
		pLaunchConfig->SetLuanchConfig(launchParam);
		LaunchConfig::PInstance = pLaunchConfig;

		return true;
	}

	/// @brief create server
	bool InitServer()
	{
		// I10n Config
		pDNl10n = std::make_shared<DNl10n>();

		if (const char* codeStr = pDNl10n->Init())
		{
			LoggerPrint()(ELogLevel_Error, codeStr);
			return false;
		}

		DNl10n::PInstance = pDNl10n;

		std::string* value = LaunchConfig::GetParam("svrType");
		EMServerType serverType = (EMServerType)stoi(*value);

		switch (serverType)
		{
			case EMServerType::ControlServer:
				pServer = std::make_unique<ControlServer>();
				break;
			case EMServerType::GlobalServer:
				pServer = std::make_unique<GlobalServer>();
				break;
			case EMServerType::AuthServer:
				pServer = std::make_unique<AuthServer>();
				break;
			case EMServerType::GateServer:
				pServer = std::make_unique<GateServer>();
				break;
			case EMServerType::DatabaseServer:
				pServer = std::make_unique<DatabaseServer>();
				break;
			case EMServerType::LogicServer:
				pServer = std::make_unique<LogicServer>();
				break;
			default:
				LoggerPrint()(EL10nCode_SrvTypeNotVaild);
				return false;
		}

		if (!pServer->Init())
		{
			return false;
		}


		pHotDll = std::make_unique<HotReloadDll>();

		if (!pHotDll->ReloadHandle())
		{
			return false;
		}

		InitCmdHandle();

		if (!OnRegHotReload())
		{
			LoggerPrint()(ELogLevel_Error, "program lunch OnRegHotReload error!");
			return false;
		}

		if (!pServer->Start())
		{
			LoggerPrint()(ELogLevel_Error, "program lunch Server Start error!");
			return false;
		}

		return true;
	}
	
	/// @brief init command line 
	void InitCmdHandle()
	{
		auto pause = [this](std::stringstream* = nullptr)
			{
				pServer->Pause();
			};

		auto resume = [this](std::stringstream* = nullptr)
			{
				pServer->Resume();
			};

		auto reload = [this, pause, resume](std::stringstream* ss = nullptr)
			{
				pause();
				OnUnregHotReload();
				pHotDll->ReloadHandle();
				OnRegHotReload();
				resume();
			};

		auto reloadConfig = [this](std::stringstream* ss = nullptr)
			{
				DNl10n::PInstance->Init();
			};

		auto open = [](std::stringstream* ss)
			{
				std::string str;
				std::string allStr = *LaunchConfig::GetParam("program") + " ";
				while (*ss >> str)
				{
					allStr += str + " ";
				}

				std::cout << allStr << std::endl;

#ifdef _WIN32
				Platform::PROCESS_INFORMATION pinfo = {};
				Platform::STARTUPINFOA startInfo = {};
				memset(&startInfo, 0, sizeof(startInfo));
				startInfo.cb = sizeof(startInfo);

				
				startInfo.dwFlags = 0x00000001; // STARTF_USESHOWWINDOW 0x00000001
				startInfo.wShowWindow = 1; // SW_SHOWNORMAL 1
				if (Platform::CreateProcessA(nullptr, allStr.data(), nullptr, nullptr, 0, 0x00000010, nullptr, nullptr, &startInfo, &pinfo)) // CREATE_NEW_CONSOLE 0x00000010
#elif __unix__
				if (0)
#endif
				{
					std::cout << "success" << std::endl;
				}
				else
				{
					std::cout << "error:" << GetLastError() << std::endl;
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

		std::string allCommands = "Commands: \n\t\t";
		for (auto& [k, v] : mCmdHandle)
		{
			allCommands += k + "\n\t\t";
		}

		LoggerPrint()(ELogLevel_Normal, allCommands);
	}	

	/// @brief exec command line
	void ExecCommand(std::string* cmd, std::stringstream* ss)
	{
		if (mCmdHandle.contains(*cmd))
		{
			mCmdHandle[*cmd](ss);
		}
	}	

	static const char* InitHotReload(){ static std::string name = GetPureFunctionName(__FUNCTION__); return name.c_str(); }

	/// @brief exec runtime lib func
	bool OnRegHotReload()
	{
		if (void* funtPtr = pHotDll->GetFuncPtr(InitHotReload()))
		{
			using funcSign = int (*)(DNServer*);
			if (funcSign func = reinterpret_cast<funcSign>(funtPtr))
			{
				return func(pServer.get()) == int(true);
			}
		}

		return false;
	}

	static const char* ShutdownHotReload(){ static std::string name = GetPureFunctionName(__FUNCTION__); return name.c_str(); }

	/// @brief exec runtime lib func
	bool OnUnregHotReload()
	{
		// launch error pHotDll is Null
		if (!pHotDll)
		{
			return false;
		}

		if (void* funtPtr = pHotDll->GetFuncPtr(ShutdownHotReload()))
		{
			using funcSign = int (*)(DNServer*);
			if (funcSign func = reinterpret_cast<funcSign>(funtPtr))
			{
				return func(pServer.get()) == int(true);
			}
		}

		return false;
	}

	void TickMainFrame() { pServer->TickMainFrame(); }

	HotReloadDll* GetHotDll() { return pHotDll.get();}
private:
	/// @brief runtime lib service pointer
	std::unique_ptr<HotReloadDll> pHotDll;

	/// @brief server service pointer
	std::unique_ptr<DNServer> pServer;

	std::shared_ptr<DNl10n> pDNl10n;

	std::shared_ptr<LaunchConfig> pLaunchConfig;

	/// @brief command line std::function mapping
	std::unordered_map<std::string, std::function<void(std::stringstream*)>> mCmdHandle;
public:
	inline static std::unique_ptr<DimensionNightmare> PInstance;
};


#pragma region Export main space 

#define REGIST_MAINSPACE_SIGN_FUNCTION(classname, methodname)\
	using classname##_##methodname##_Sign = decltype(&classname::methodname);\
	using classname##_##methodname##_Args = typename MemberFunctionArgs<classname##_##methodname##_Sign>::Arguments;\
	__declspec(dllexport) auto classname##_##methodname(classname *obj, classname##_##methodname##_Args args)\
	{\
		return apply([&obj](auto &&...unpack) { return obj->methodname(std::forward<decltype(unpack)>(unpack)...); }, args);\
	}

extern "C"
{
	REGIST_MAINSPACE_SIGN_FUNCTION(DNl10n, GetInstance);
	REGIST_MAINSPACE_SIGN_FUNCTION(LaunchConfig, GetInstance);

	REGIST_MAINSPACE_SIGN_FUNCTION(ProxyEntityManager, CheckEntityCloseTimer);
	REGIST_MAINSPACE_SIGN_FUNCTION(RoomEntityManager, CheckEntityCloseTimer);
	REGIST_MAINSPACE_SIGN_FUNCTION(ServerEntityManager, CheckEntityCloseTimer);
	REGIST_MAINSPACE_SIGN_FUNCTION(DNClientProxy, InitConnectedChannel);
	REGIST_MAINSPACE_SIGN_FUNCTION(DNClientProxy, CheckMessageTimeoutTimer);
	REGIST_MAINSPACE_SIGN_FUNCTION(DNClientProxy, RedirectClient);
	REGIST_MAINSPACE_SIGN_FUNCTION(DNServerProxy, InitConnectedChannel);
	REGIST_MAINSPACE_SIGN_FUNCTION(DNServerProxy, CheckMessageTimeoutTimer);
}


#pragma endregion
