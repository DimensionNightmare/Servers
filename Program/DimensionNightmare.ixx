export module DimensionNightmare;

import Server;
import ThirdParty.Platform;
import HotReload;
import ProxyEntityManager;
import RoomEntityManager;
import ServerEntityManager;
import ClientEntityManager;
import ClientProxy;
import ServerProxy;
import WebProxy;
import StrUtils;
import RdbProxy;
import MdbProxy;
import BitFlag;
import std.compat;
import ECSW;
import Logger;
import Timer;

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
	if (hDumpFile == nullptr)
	{
		return;
	}

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

export enum class EMProgramFlag
{
	None = 0,
	ResourceLoadDown = 1,
	Max,
};


export class DimensionNightmare : public World, public BitFlag<EMProgramFlag>
{

public:
	/// @brief
	DimensionNightmare()
	{
	}

	// need close main process
	~DimensionNightmare()
	{
		// Dispose();
	}

	/// @brief load ini config
	bool Init(std::unordered_map<std::string, std::string> launchParam)
	{
		/// @brief load ini config
		BitFlag<EMServerType> bitServerOpenFlag;
		std::unordered_map<std::string, std::unordered_map<std::string, std::string>> iniFileParam;

#pragma region LuanchConfig

		if (!launchParam.contains("svrType"))
		{
			SPidLogger->Record(ELogLevel_Error, "lunch param svrType is null! ");
			return false;
		}

		for(auto& serverType : StrSplit(launchParam["svrType"], ","))
		{
			bitServerOpenFlag.SetFlag(std::stoi(serverType));
		}

		launchParam.erase("svrType");

		uint64_t bitFlagValue = bitServerOpenFlag.GetAllFlagNum();
		if (bitFlagValue == 0 || bitFlagValue >= (1 << static_cast<uint8_t>(EMServerType::Max)))
		{
			SPidLogger->Record(ELogLevel_Error, "serverType Not Invalid! ");
			return false;
		}

#ifndef NDEBUG
		const char* iniFilePath = "./Config/ServerDebug.ini";
#else
		const char* iniFilePath = "./Config/Server.ini";
#endif

		if(!std::filesystem::exists(iniFilePath))
		{
			SPidLogger->Record(ELogLevel_Error, "ConfigIni Not Finded!");
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

		// muti dnServer only this valid.
		if(bitServerOpenFlag.GetAllFlagCount() > 1)
		{
			iniFileParam["Common"]["program"] = launchParam["program"];
		}
		else
		{
			launchParam.merge(iniFileParam["Common"]);
			iniFileParam["Common"] = std::move(launchParam);
		}
		
		auto mapCopy = iniFileParam["Common"];
		MoveLuanchConfigToSelf(mapCopy);

		SPidLogger->Init(iniFileParam["Common"]);
		
		HotReload::CVPtr pHotDll = AddSystem<HotReload>();

		if (!pHotDll->ReloadHandle())
		{
			return false;
		}
	
		for(auto& [serverEnum, serverName] : ServerTypeList)
		{
			// set global Launch config
			if(bitServerOpenFlag.HasFlag(serverEnum))
			{
				auto mergeMap = iniFileParam["Common"];
				mergeMap.merge(iniFileParam[serverName]);

				World::CVPtr world = MemPool->Allocate<World>();
				world->MoveLuanchConfigToSelf(mergeMap);

				if(!InitServer(world, pHotDll))
				{
					world->Dispose();
					continue;
				}

				world->Broadcast(EMEventType::ServerStart);

				oWorlds.push_back(world);
			}
		}

		// free manager
		// RemoveSystem(EMSystemType::HotReload);

		return true;
	}

	/// @brief create dnServer
	/// @param pHotDll if mutiServer, will own common
	bool InitServer(World::CVPtr world, HotReload::CVPtr pHotDll)
	{
		
		// logger
		LoggerPrint::CVPtr pLogger = world->AddSystem<LoggerPrint>();
		if(!pLogger)
		{
			return false;
		}

		// i10n
		L10nText::CVPtr dnL10n = world->AddSystem<L10nText>();
		if(!dnL10n)
		{
			return false;
		}

		world->AddSystem<Timer>();

		
		std::string* value = world->LaunchParam("svrName");
		EMServerType serverType = EnumName<EMServerType>(*value);

		Server::CVPtr dnServer = world->AddSystem<Server>();
		dnServer->SetServerType(serverType);

		value = world->LaunchParam("byCtl");

		switch (serverType)
		{
			case EMServerType::ControlServer:
			{
				dnServer->AddComponent<ServerEntityManager>();
				//net
				dnServer->AddComponent<ServerProxy>();
				break;
			}
			case EMServerType::GlobalServer:
			{
				dnServer->AddComponent<ServerEntityManager>();
				//net
				dnServer->AddComponent<ServerProxy>();
				if(value)
				{
					dnServer->AddComponent<ClientProxy>();
				}
				break;
			}
			case EMServerType::AuthServer:
			{
				// db
				dnServer->AddComponent<RdbProxy>();
				dnServer->AddComponent<WebProxy>();
				//net
				if(value)
				{
					dnServer->AddComponent<ClientProxy>();
				}
				break;
			}
			case EMServerType::GateServer:
			{
				dnServer->AddComponent<ServerEntityManager>();
				dnServer->AddComponent<ProxyEntityManager>();
				//net
				dnServer->AddComponent<ServerProxy>();
				dnServer->AddComponent<ClientProxy>();
				break;
			}
			case EMServerType::DatabaseServer:
			{
				// db
				dnServer->AddComponent<RdbProxy>();
				//net
				dnServer->AddComponent<ClientProxy>();
				break;
			}
			case EMServerType::LogicServer:
			{
				// db
				dnServer->AddComponent<MdbProxy>();
				dnServer->AddComponent<RoomEntityManager>();
				dnServer->AddComponent<ClientEntityManager>();
				//net
				dnServer->AddComponent<ServerProxy>();
				dnServer->AddComponent<ClientProxy>();
				break;
			}
			default:
			{
				pLogger->Record(EL10nCode_SrvTypeNotVaild);
				return false;
			}
		}

		try
		{
			dnServer->Broadcast(EMEventType::ServerStart);
		}
		catch(const std::exception& e)
		{
			pLogger->Record(ELogLevel_Error, "dnserver lunch error! error: {}", e.what());
			return false;
		}
		
		if (pHotDll->pInitHotReload(world) != 1)
		{
			pLogger->Record(ELogLevel_Error, "program lunch OnRegHotReload error!");
			return false;
		}

		InitCmdHandle();

		return true;
	}
	
	/// @brief init command line 
	void InitCmdHandle()
	{
		auto pause = [](std::stringstream* = nullptr)
			{
				GEvent.Broadcast(EMEventType::ServerPause	);
			};

		auto resume = [](std::stringstream* = nullptr)
			{
				GEvent.Broadcast(EMEventType::ServerResume);
			};

		auto reloadDll = [this, pause, resume](std::stringstream* ss = nullptr)
			{
				pause();
				
				HotReload::CVPtr pHotDll = GetSystem<HotReload>(EMSystemType::HotReload);

				//after func
				auto unloadFunc = pHotDll->pShutdownHotReload;

				if(pHotDll->ReloadHandle([&](){

					for(auto& world : oWorlds)
					{
						unloadFunc(world);
					}

					unloadFunc = nullptr;
				}))
				{
					for(auto& world : oWorlds)
					{
						pHotDll->pInitHotReload(world);
					}
				}
				
				resume();
			};

		auto reloadConfig = [this](std::stringstream* = nullptr)
			{
				// L10nText::PInstance->Init();
			};

		mCmdHandle = {
			#define one(func) {#func, func}
			
			one(pause), one(resume), one(reloadDll),
			one(reloadConfig)
			
			#undef one
		};

		// std::string allCommands = "Commands: \n\t\t";
		// for (auto& [k, v] : mCmdHandle)
		// {
		// 	allCommands += k + "\n\t\t";
		// }

		// SPidLogger->Record(ELogLevel_Normal, "{}", allCommands);
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

	virtual void Dispose() override
	{
		for (auto it = oWorlds.rbegin(); it != oWorlds.rend(); ++it)
		{
			(*it)->Dispose();
		}
		
		oWorlds.clear();
		
		World::Dispose();

		mCmdHandle.clear();
	}

private:

	std::vector<World::Ptr> oWorlds;

	/// @brief command line std::function mapping
	std::unordered_map<std::string, std::function<void(std::stringstream*)>> mCmdHandle;
public:
	inline static std::shared_ptr<DimensionNightmare> PInstance;
};


#pragma region Export main space 

extern "C"
{
	__declspec(dllexport) World* GetMainWorld()
	{
		return DimensionNightmare::PInstance.get();
	}
}

// template <typename Method>
// struct MemberFunctionArgs;

// template <typename R, typename Class, typename... Args>
// struct MemberFunctionArgs<R(Class::*)(Args...)>
// {
// 	using Arguments = std::tuple<Args...>;
// };

// #define REGIST_MAINSPACE_SIGN_FUNCTION(Class, Method) 																		\
//     __declspec(dllexport) auto Class##_##Method(Class* obj, MemberFunctionArgs<decltype(&Class::Method)>::Arguments args)	\
//     {																														\
// 		return std::apply([obj](auto&&... args) {																			\
//             return std::invoke(&Class::Method, obj, std::forward<decltype(args)>(args)...);									\
//         }, args);																											\
//     }

// extern "C"
// {
// 	REGIST_MAINSPACE_SIGN_FUNCTION(ClientProxy, RedirectClient);
// }

#pragma endregion
