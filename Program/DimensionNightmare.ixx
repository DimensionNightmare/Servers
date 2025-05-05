module;

export module DimensionNightmare;

import StrUtils;
import L10nText;
import Logger;
import Config.Server;
import ThirdParty.PbGen;
import DNServer;
import DllUtils;
import Platform;
import HotReloadDll;
import ECSW;
import ProxyEntityManager;
import RoomEntityManager;
import ServerEntityManager;
import DNClientProxy;
import DNServerProxy;

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
	}

	/// @brief load ini config
	bool Init(std::unordered_map<std::string, std::string>&& launchParam)
	{
		ServerTypeBitFlag bitFlag;
		
		for(auto& serverType : StrSplit(launchParam["svrType"], ","))
		{
			bitFlag.set(stoi(serverType));
		}

		launchParam.erase("svrType");

		uint64_t bitFlagValue = bitFlag.to_ulong();
		if (bitFlagValue == 0 || bitFlagValue >= (1 << static_cast<uint8_t>(EMServerType::Max)))
		{
			SPidLogger.Record(ELogLevel_Error, "serverType Not Invalid! ");
			return false;
		}

#ifndef NDEBUG
		const char* iniFilePath = "./Config/ServerDebug.ini";
#else
		const char* iniFilePath = "./Config/Server.ini";
#endif

		if(!std::filesystem::exists(iniFilePath))
		{
			SPidLogger.Record(ELogLevel_Error, "ConfigIni Not Finded!");
			return false;
		}

		// get ini Config
		std::unordered_map<std::string, std::unordered_map<std::string, std::string>> sectionNames;

#ifdef _WIN32
		#define MAX_SECTION_NAME 512
		char buffer[MAX_SECTION_NAME] = { 0 };
		size_t bufferSize = sizeof(buffer);
		Platform::GetPrivateProfileSectionNamesA(buffer, MAX_SECTION_NAME, iniFilePath);
		char* current = buffer;
		while (*current)
		{
			sectionNames[current];
			current += strlen(current) + 1;

			// if (sectionNames.back().find_last_of("Server") != std::string::npos && sectionNames.back() != serverName)
			// {
			// 	sectionNames.pop_back();
			// }
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
		auto handler = [&](std::unordered_map<std::string, std::string>& map, std::string& split)
		{
			size_t pos = split.find('=');
			if (pos != std::string::npos)
			{
				std::string key = split.substr(0, pos);
				// if (launchParam.contains(key))
				// {
				// 	return;
				// }

				map.emplace(key, split.substr(pos + 1));
			}
		};
		

		for (auto& [sectionName, sectionMap] : sectionNames)
		{
			if (sectionName.find_last_of("Server") != std::string::npos)
			{
				sectionMap.emplace("svrName", sectionName);
			}

#ifdef _WIN32
			Platform::GetPrivateProfileSectionA(sectionName.c_str(), buffer, MAX_SECTION_NAME, iniFilePath);
			char* keyValuePair = buffer;
			while (*keyValuePair)
			{
				std::string split(keyValuePair);
				keyValuePair += strlen(keyValuePair) + 1;
				handler(sectionMap, split);
			}
#elif __unix__
			for (const std::string& keyValuePair : sectionVal[sectionName])
			{
				std::string split(keyValuePair);
				handler(split);
			}
#endif
		}

		// muti server only this valid.
		if(bitFlag.count() > 1)
		{
			sectionNames["Common"]["program"] = launchParam["program"];
		}
		else
		{
			launchParam.merge(sectionNames["Common"]);
			sectionNames["Common"] = std::move(launchParam);
		}

		for(auto& [serverEnum, serverName] : ServerTypeList)
		{
			// set global Launch config
			if(bitFlag.test(serverEnum))
			{
				auto merge = sectionNames["Common"];
				merge.merge(sectionNames[serverName]);
				if(!InitServer(std::move(merge)))
				{
					return false;
				}
			}
		}

		return true;
	}

	/// @brief create server
	bool InitServer(std::unordered_map<std::string, std::string>&& launchParam)
	{

		World::Ptr world = std::make_shared<World>();
		oWorlds.push_back(world);

		world->MoveLuanchConfigToSelf(std::move(launchParam));

		LoggerPrint::Ptr logger = world->AddSystem<LoggerPrint>();
		world->AddSystem<DNl10n>();
		DNServer::Ptr server = world->AddSystem<DNServer>();
		
		std::string* value = world->LuanchParam("svrName");
		EMServerType serverType = EnumName<EMServerType>(*value);

		switch (serverType)
		{
			case EMServerType::ControlServer:
			{
				server->AddComponent<DNServerProxy>();
				// pServer = std::make_unique<ControlServer>();
				break;
			}
			case EMServerType::GlobalServer:
			{

				// pServer = std::make_unique<GlobalServer>();
				break;
			}
			case EMServerType::AuthServer:
			{

				// pServer = std::make_unique<AuthServer>();
				break;
			}
			case EMServerType::GateServer:
			{

				// pServer = std::make_unique<GateServer>();
				break;
			}
			case EMServerType::DatabaseServer:
			{

				// pServer = std::make_unique<DatabaseServer>();
				break;
			}
			case EMServerType::LogicServer:
			{

				// pServer = std::make_unique<LogicServer>();
				break;
			}
			default:
			{
				logger->Record(EL10nCode_SrvTypeNotVaild);
				return false;
			}
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
			logger->Record(ELogLevel_Error, "program lunch OnRegHotReload error!");
			return false;
		}

		if (!pServer->Start())
		{
			logger->Record(ELogLevel_Error, "program lunch Server Start error!");
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

		mCmdHandle = {
			#define one(func) {#func, func}
			
			one(pause), one(resume), one(reload),
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

		SPidLogger.Record(ELogLevel_Normal, allCommands);
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

	std::vector<World::Ptr> oWorlds;

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
