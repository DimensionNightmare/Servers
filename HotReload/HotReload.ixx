
module;
#ifdef _WIN32
	#include <windef.h>
#endif
#include <functional>
#include <string>

#include "StdMacro.h"
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

#ifdef _WIN32
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	// Perform actions based on the reason for calling.
	switch (fdwReason)
	{
		case DLL_PROCESS_DETACH:
			if (lpvReserved != nullptr)
			{
				break;
			}
			break;
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:

			break;
	}
	return TRUE;
}
#endif

extern "C"
{
	HOTRELOAD int InitHotReload(DNServer* server)
	{
		HV_hlog_disable();

		SetLuanchConfig(server->pLuanchConfig);
		SetDNl10nInstance(server->pDNl10nInstance, true);

		ServerType servertype = server->GetServerType();
		string_view serverName = EnumName(servertype);
		SetLoggerLevel(LoggerLevel::Debug, serverName);

		bool isDeal = false;
		switch (servertype)
		{
			case ServerType::ControlServer:
				isDeal = HandleControlServerInit(server);
				break;
			case ServerType::GlobalServer:
				isDeal = HandleGlobalServerInit(server);
				break;
			case ServerType::AuthServer:
				isDeal = HandleAuthServerInit(server);
				break;
			case ServerType::GateServer:
				isDeal = HandleGateServerInit(server);
				break;
			case ServerType::DatabaseServer:
				isDeal = HandleDatabaseServerInit(server);
				break;
			case ServerType::LogicServer:
				isDeal = HandleLogicServerInit(server);
				break;
			default:
				break;
		}

		return isDeal;
	}

	HOTRELOAD int ShutdownHotReload(DNServer* server)
	{
		ServerType servertype = server->GetServerType();
		bool isDeal = false;
		switch (servertype)
		{
			case ServerType::ControlServer:
				isDeal = HandleControlServerShutdown(server);
				break;
			case ServerType::GlobalServer:
				isDeal = HandleGlobalServerShutdown(server);
				break;
			case ServerType::AuthServer:
				isDeal = HandleAuthServerShutdown(server);
				break;
			case ServerType::GateServer:
				isDeal = HandleGateServerShutdown(server);
				break;
			case ServerType::DatabaseServer:
				isDeal = HandleDatabaseServerShutdown(server);
				break;
			case ServerType::LogicServer:
				isDeal = HandleLogicServerShutdown(server);
				break;
			default:
				break;
		}

		PB_ShutdownProtobufLibrary();
		HV_cleanup();

		return isDeal;
	}
}
