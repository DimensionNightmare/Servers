module;

export module DimensionNightmare;

import Logger;
import ThirdParty.PbGen;
import DNServer;
import DllUtils;
import ThirdParty.Platform;
import HotReloadDll;
import ECSW;
import ProxyEntityManager;
import RoomEntityManager;
import ServerEntityManager;
import ClientEntityManager;
import DNClientProxy;
import DNServerProxy;
import DNWebProxy;
import StrUtils;
import RdbProxy;

export void WriteDumpFile(std::filesystem::path fileName, _EXCEPTION_POINTERS* ExceptionInfo = nullptr)
{
	auto hDumpFile = Platform::CreateFileA(
		fileName.string().c_str(),
		0x40000000L, // GENERIC_WRITE
		0,
		nullptr,
		2, // CREATE_ALWAYS
		0x00000080, // FILE_ATTRIBUTE_NORMAL
		nullptr
	);

	// INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
	if (hDumpFile != (void*)(int64_t*)-1)
	{
		Platform::_MINIDUMP_EXCEPTION_INFORMATION info;
		info.ThreadId = Platform::GetCurrentThreadId();
		info.ExceptionPointers = ExceptionInfo;
		info.ClientPointers = 0;

		Platform::MINIDUMP_TYPE dumpType = (Platform::MINIDUMP_TYPE)(
			MiniDumpWithDataSegs |
			MiniDumpWithFullMemory |
			MiniDumpWithHandleData |
			MiniDumpWithThreadInfo |
			MiniDumpWithUnloadedModules |
			MiniDumpWithFullMemoryInfo |
			MiniDumpWithProcessThreadData
			);

		Platform::MiniDumpWriteDump(
			Platform::GetCurrentProcess(),
			Platform::GetCurrentProcessId(),
			hDumpFile,
			dumpType, // MiniDumpNormal
			ExceptionInfo ? &info : nullptr,
			nullptr,
			nullptr
		);

		Platform::CloseHandle(hDumpFile);
	}
}


export class DimensionNightmare : public World
{

public:
	/// @brief
	DimensionNightmare()
	{
	}

	// need close main process
	~DimensionNightmare()
	{
		Dispose();
	}

	/// @brief load ini config
	bool Init(std::unordered_map<std::string, std::string>&& launchParam)
	{
		/// @brief load ini config
		ServerTypeBitFlag bitServerOpenFlag;
		std::unordered_map<std::string, std::unordered_map<std::string, std::string>> iniFileParam;

#pragma region LuanchConfig

		if (!launchParam.contains("svrType"))
		{
			SPidLogger.Record(ELogLevel_Error, "lunch param svrType is null! ");
			return false;
		}

		for(auto& serverType : StrSplit(launchParam["svrType"], ","))
		{
			bitServerOpenFlag.set(stoi(serverType));
		}

		launchParam.erase("svrType");

		uint64_t bitFlagValue = bitServerOpenFlag.to_ulong();
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

#ifdef _WIN32
		#define MAX_SECTION_NAME 512
		char buffer[MAX_SECTION_NAME] = { 0 };
		size_t bufferSize = sizeof(buffer);
		Platform::GetPrivateProfileSectionNamesA(buffer, MAX_SECTION_NAME, iniFilePath);
		char* current = buffer;
		while (*current)
		{
			iniFileParam[current];
			current += strlen(current) + 1;
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
							std::string mainSection = line.substr(1, endPos - 1);
							iniFileParam.emplace_back(mainSection);
						}
					}
					else if (line[0] != ';')
					{
						sectionVal[iniFileParam.back()].emplace_back(line);
					}
				}

				file.close();
			};

		GetINISectionNames(iniFilePath);

		auto iter = iniFileParam.begin();
		while (iter != iniFileParam.end())
		{
			if ((*iter).find("Server") != std::string::npos && *iter != serverName)
			{
				sectionVal.erase(*iter);
				iter = iniFileParam.erase(iter);
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

				map.emplace(key, split.substr(pos + 1));
			}
		};
		

		for (auto& [mainSection, sectionMap] : iniFileParam)
		{
			if (mainSection.find_last_of("Server") != std::string::npos)
			{
				sectionMap.emplace("svrName", mainSection);
			}

#ifdef _WIN32
			Platform::GetPrivateProfileSectionA(mainSection.c_str(), buffer, MAX_SECTION_NAME, iniFilePath);
			char* keyValuePair = buffer;
			while (*keyValuePair)
			{
				std::string split(keyValuePair);
				keyValuePair += strlen(keyValuePair) + 1;
				handler(sectionMap, split);
			}
#elif __unix__
			for (const std::string& keyValuePair : sectionVal[mainSection])
			{
				std::string split(keyValuePair);
				handler(split);
			}
#endif
		}

#pragma endregion

		HotReloadDll::Ptr pHotDll;

		// muti server only this valid.
		if(bitServerOpenFlag.count() > 1)
		{
			iniFileParam["Common"]["program"] = launchParam["program"];

			MoveLuanchConfigToSelf(iniFileParam["Common"]);

			pHotDll = AddSystem<HotReloadDll>();

			if (!pHotDll->ReloadHandle())
			{
				return false;
			}
		}
		else
		{
			launchParam.merge(iniFileParam["Common"]);
			iniFileParam["Common"] = std::move(launchParam);
		}


		SPidLogger.Init(iniFileParam["Common"]);
	
		for(auto& [serverEnum, serverName] : ServerTypeList)
		{
			// set global Launch config
			if(bitServerOpenFlag.test(serverEnum))
			{
				auto mergeMap = iniFileParam["Common"];
				mergeMap.merge(iniFileParam[serverName]);

				World::Ptr world = std::make_shared<World>();
				world->MoveLuanchConfigToSelf(std::move(mergeMap));

				if(!InitServer(world, pHotDll))
				{
					world->Dispose();
					return false;
				}

				oWorlds.push_back(world);
			}
		}


		MoveLuanchConfigToSelf(std::move(iniFileParam["Common"]));

		// free manager
		RemoveSystem(EMSystemType::HotReloadDll);

		return true;
	}

	/// @brief create server
	/// @param pHotDll if mutiServer, will own common
	bool InitServer(World::Ptr world, HotReloadDll::Ptr pHotDll)
	{
		
		// logger
		LoggerPrint::Ptr pLogger = world->AddSystem<LoggerPrint>();
		if(!pLogger->Init())
		{
			return false;
		}

		// i10n
		DNl10n::Ptr dnL10n = world->AddSystem<DNl10n>();
		if(!dnL10n->Init())
		{
			return false;
		}

		
		std::string* value = world->LaunchParam("svrName");
		EMServerType serverType = EnumName<EMServerType>(*value);

		DNServer::Ptr server = world->AddSystem<DNServer>();
		server->SetServerType(serverType);

		value = world->LaunchParam("byCtl");

		switch (serverType)
		{
			case EMServerType::ControlServer:
			{
				server->AddComponent<DNServerProxy>();
				server->AddComponent<ServerEntityManager>();
				break;
			}
			case EMServerType::GlobalServer:
			{
				server->AddComponent<DNServerProxy>();
				if(value)
				{
					server->AddComponent<DNClientProxy>();
				}
				server->AddComponent<ServerEntityManager>();
				break;
			}
			case EMServerType::AuthServer:
			{
				server->AddComponent<DNWebProxy>();
				if(value)
				{
					server->AddComponent<DNClientProxy>();
				}
				server->AddComponent<RdbProxy>();
				break;
			}
			case EMServerType::GateServer:
			{
				server->AddComponent<DNServerProxy>();
				server->AddComponent<DNClientProxy>();
				server->AddComponent<ServerEntityManager>();
				server->AddComponent<ProxyEntityManager>();
				break;
			}
			case EMServerType::DatabaseServer:
			{
				server->AddComponent<DNClientProxy>();
				break;
			}
			case EMServerType::LogicServer:
			{
				server->AddComponent<DNServerProxy>();
				server->AddComponent<DNClientProxy>();
				server->AddComponent<RoomEntityManager>();
				server->AddComponent<ClientEntityManager>();
				break;
			}
			default:
			{
				pLogger->Record(EL10nCode_SrvTypeNotVaild);
				return false;
			}
		}

		if(pHotDll)
		{
			world->AddSystem(pHotDll);
		}
		else
		{
			pHotDll = world->AddSystem<HotReloadDll>();

			if (!pHotDll->ReloadHandle())
			{
				return false;
			}
		}

		if (!pHotDll->OnRegHotReload(world))
		{
			pLogger->Record(ELogLevel_Error, "program lunch OnRegHotReload error!");
			return false;
		}

		InitCmdHandle();

		server->Broadcast(EMEventType::ServerStart);

		return true;
	}
	
	/// @brief init command line 
	void InitCmdHandle()
	{
		auto pause = [this](std::stringstream* = nullptr)
			{
				GEvent.Broadcast(EMEventType::ServerPause	);
			};

		auto resume = [this](std::stringstream* = nullptr)
			{
				GEvent.Broadcast(EMEventType::ServerResume);
			};

		auto reloadDll = [this, pause, resume](std::stringstream* ss = nullptr)
			{
				pause();
				
				resume();
			};

		auto reloadConfig = [this](std::stringstream* ss = nullptr)
			{
				// DNl10n::PInstance->Init();
			};

		mCmdHandle = {
			#define one(func) {#func, func}
			
			one(pause), one(resume), one(reloadDll),
			one(reloadConfig)
			
			#undef one
		};

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

	void TickMainFrame() {  }

	/// @brief init gWorldWPtr with world ptr
	void AppStartInitThread()
	{
		
	}

	void Dispose() override
	{
		Object::Dispose();

		for (auto& world : oWorlds)
		{
			world->Dispose();
		}
		
		oWorlds.clear();
	}

private:

	std::vector<World::Ptr> oWorlds;

	/// @brief command line std::function mapping
	std::unordered_map<std::string, std::function<void(std::stringstream*)>> mCmdHandle;
public:
	inline static std::shared_ptr<DimensionNightmare> PInstance;
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
