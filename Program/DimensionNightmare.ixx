module;

export module DimensionNightmare;

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
import StrUtils;

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
	bool Init(ServerTypeBitFlag& bitFlag, std::unordered_map<std::string, std::unordered_map<std::string, std::string>>&& inIniFileParam)
	{
		auto iniFileParam = std::move(inIniFileParam);
		// drop up
	
		for(auto& [serverEnum, serverName] : ServerTypeList)
		{
			// set global Launch config
			if(bitFlag.test(serverEnum))
			{
				auto merge = iniFileParam["Common"];
				merge.merge(iniFileParam[serverName]);
				if(!InitServer(std::move(merge)))
				{
					return false;
				}
			}
		}

		return true;
	}

	/// @brief create server
	bool InitServer(std::unordered_map<std::string, std::string>&& iniServerParam)
	{

		World::Ptr world = std::make_shared<World>();
		oWorlds.push_back(world);

		world->MoveLuanchConfigToSelf(std::move(iniServerParam));

		LoggerPrint::Ptr logger = world->AddSystem<LoggerPrint>();
		DNl10n::Ptr dnL10n = world->AddSystem<DNl10n>();

		if(const char* errInfo = dnL10n->Init())
		{
			
			return false;
		}

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
