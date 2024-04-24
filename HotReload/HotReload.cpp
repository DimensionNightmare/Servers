
// module;
#ifdef _WIN32
	#include <windef.h>
#endif
#include <functional>
#include <string>
#include "google/protobuf/message.h"
#include "hv/hasync.h"

#include "StdAfx.h"
// export module HotReload;

import DNServer;
import GlobalServerInit;
import ControlServerInit;
import AuthServerInit;
import GateServerInit;
import DatabaseServerInit;
import LogicServerInit;
import DNClientProxyHelper;

#ifdef HOTRELOAD_BUILD
#define HOTRELOAD __declspec(dllexport)
#else
#define HOTRELOAD __declspec(dllimport)
#endif

#ifdef _WIN32
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    // Perform actions based on the reason for calling.
    switch (fdwReason)
    {
    case DLL_PROCESS_DETACH:
		google::protobuf::ShutdownProtobufLibrary();
		hv::async::cleanup();
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
	HOTRELOAD int InitHotReload(DNServer* server);
	HOTRELOAD int ShutdownHotReload(DNServer* server);
}

int InitHotReload(DNServer* server)
{
	SetLuanchConfig(server->pLuanchConfig);
	SetDNl10nInstance(server->pDNl10nInstance);

	SetLoggerLevel(LoggerLevel::Debug);
	
	ServerType servertype = server->GetServerType();
	switch (servertype)
	{
	case ServerType::ControlServer:
		return HandleControlServerInit(server);
	case ServerType::GlobalServer:
		return HandleGlobalServerInit(server);
	case ServerType::AuthServer:
		return HandleAuthServerInit(server);
	case ServerType::GateServer:
		return HandleGateServerInit(server);
	case ServerType::DatabaseServer:
		return HandleDatabaseServerInit(server);
	case ServerType::LogicServer:
		return HandleLogicServerInit(server);
	}

	return 0;
}

int ShutdownHotReload(DNServer *server)
{
	ServerType servertype = server->GetServerType();
	switch (servertype)
	{
	case ServerType::ControlServer:
		return HandleControlServerShutdown(server);
	case ServerType::GlobalServer:
		return HandleGlobalServerShutdown(server);
	case ServerType::AuthServer:
		return HandleAuthServerShutdown(server);
	case ServerType::GateServer:
		return HandleGateServerShutdown(server);
	case ServerType::DatabaseServer:
		return HandleDatabaseServerShutdown(server);
	case ServerType::LogicServer:
		return HandleLogicServerShutdown(server);
	}

	return 0;
}
