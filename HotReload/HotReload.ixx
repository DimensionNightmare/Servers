
module;

export module HotReload;

import DNServer;
import GlobalServerInit;
import ControlServerInit;
import AuthServerInit;
import GateServerInit;
import DatabaseServerInit;
import LogicServerInit;
import DNClientProxyHelper;
import Logger;
import Config.Server;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import StrUtils;
import Platform;
import std.compat;

#ifdef _WIN32
	#ifdef HOTRELOAD_BUILD
		#define HOTRELOAD __declspec(dllexport)
	#else
		#define HOTRELOAD __declspec(dllimport)
	#endif
#elif __unix__
	#ifdef HOTRELOAD_BUILD
		#define HOTRELOAD __attribute__((visibility("default")))
	#else
		#define HOTRELOAD
	#endif
#endif


extern "C"
{
	

#ifdef _WIN32
	int DllMain(HMODULE hinstDLL, uint32_t fdwReason, void* lpvReserved)
	{
		// Perform actions based on the reason for calling.
		switch (fdwReason)
		{
			// DLL_PROCESS_DETACH
			case 0:
				if (lpvReserved != nullptr)
				{
					break;
				}
				break;
			// DLL_PROCESS_ATTACH
			case 1:
				break;
			// DLL_THREAD_ATTACH
			case 2:
				break;
			// DLL_THREAD_DETACH
			case 3:

				break;
		}
		return 1;
	}
#endif

	HOTRELOAD int InitHotReload(DNServer* server)
	{
		hvlog_disable();

		EMServerType servertype = server->GetServerType();
		std::string_view serverName = EnumName(servertype);
		if(std::string* value = LaunchConfig::GetParam("program"))
		{
			std::filesystem::path envPath = std::filesystem::path(*value).parent_path().append(serverName);
			ELogLevel logLevel = ELogLevel_Debug;
			value = LaunchConfig::GetParam("LoggerLevel");
			if(value && ELogLevel_Parse(*value, &logLevel))
			{
				
			}
			LoggerPrint::SetLoggerLevel(logLevel, envPath);
		}

		bool isDeal = false;
		switch (servertype)
		{
			case EMServerType::ControlServer:
				isDeal = HandleControlServerInit(server);
				break;
			case EMServerType::GlobalServer:
				isDeal = HandleGlobalServerInit(server);
				break;
			case EMServerType::AuthServer:
				isDeal = HandleAuthServerInit(server);
				break;
			case EMServerType::GateServer:
				isDeal = HandleGateServerInit(server);
				break;
			case EMServerType::DatabaseServer:
				isDeal = HandleDatabaseServerInit(server);
				break;
			case EMServerType::LogicServer:
				isDeal = HandleLogicServerInit(server);
				break;
			default:
				break;
		}

		return isDeal;
	}

	HOTRELOAD int ShutdownHotReload(DNServer* server)
	{
		EMServerType servertype = server->GetServerType();
		bool isDeal = false;
		switch (servertype)
		{
			case EMServerType::ControlServer:
				isDeal = HandleControlServerShutdown(server);
				break;
			case EMServerType::GlobalServer:
				isDeal = HandleGlobalServerShutdown(server);
				break;
			case EMServerType::AuthServer:
				isDeal = HandleAuthServerShutdown(server);
				break;
			case EMServerType::GateServer:
				isDeal = HandleGateServerShutdown(server);
				break;
			case EMServerType::DatabaseServer:
				isDeal = HandleDatabaseServerShutdown(server);
				break;
			case EMServerType::LogicServer:
				isDeal = HandleLogicServerShutdown(server);
				break;
			default:
				break;
		}

		ShutdownProtobufLibrary();
		cleanup();

		return isDeal;
	}
}
