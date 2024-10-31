
module;
#ifdef _WIN32
	#include <windef.h>
#endif
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
		HVExport::hlog_disable();

		SetLuanchConfig(server->pLuanchConfig);
		SetDNl10nInstance(server->pDNl10nInstance);

		EMServerType servertype = server->GetServerType();
		string_view serverName = EnumName(servertype);
		SetLoggerLevel(EMLoggerLevel::Debug, serverName);

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

		PBExport::ShutdownProtobufLibrary();
		HVExport::cleanup();

		return isDeal;
	}
}
