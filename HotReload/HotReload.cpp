
#include "google/protobuf/message.h"

#include <Windows.h>
#include <functional>
#include <string>

import DNServer;
import DimensionNightmare;
import GlobalServerInit;
import ControlServerInit;
import AuthServerInit;
import GateServerInit;
import DatabaseServerInit;
import LogicServerInit;

import DNClientProxyHelper;
import AfxCommon;

#ifdef HOTRELOAD_BUILD
#define HOTRELOAD __declspec(dllexport)
#else
#define HOTRELOAD __declspec(dllimport)
#endif

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    // Perform actions based on the reason for calling.
    switch (fdwReason)
    {
    case DLL_PROCESS_DETACH:
		google::protobuf::ShutdownProtobufLibrary();
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

extern "C"
{
	HOTRELOAD int InitHotReload(DimensionNightmare &mainObj);
	HOTRELOAD int ShutdownHotReload(DimensionNightmare &mainObj);

	HOTRELOAD int RegClientReconnectFunc(std::function<void(const char*, int)> func);
}

int InitHotReload(DimensionNightmare &mainObj)
{
	mainObj.InitDllEnv();

	SetLoggerLevel(LoggerLevel::Debug);
	
	DNServer* base = mainObj.GetServer();
	ServerType servertype = base->GetServerType();
	switch (servertype)
	{
	case ServerType::ControlServer:
		HandleControlServerInit(base);
		break;
	case ServerType::GlobalServer:
		HandleGlobalServerInit(base);
		break;
	case ServerType::AuthServer:
		HandleAuthServerInit(base);
		break;
	case ServerType::GateServer:
		HandleGateServerInit(base);
		break;
	case ServerType::DatabaseServer:
		HandleDatabaseServerInit(base);
		break;
	case ServerType::LogicServer:
		HandleLogicServerInit(base);
		break;
	}

	return 0;
}

int ShutdownHotReload(DimensionNightmare &mainObj)
{
	DNServer* base = mainObj.GetServer();
	ServerType servertype = base->GetServerType();
	switch (servertype)
	{
	case ServerType::ControlServer:
		HandleControlServerShutdown(base);
		break;
	case ServerType::GlobalServer:
		HandleGlobalServerShutdown(base);
		break;
	case ServerType::AuthServer:
		HandleAuthServerShutdown(base);
		break;
	case ServerType::GateServer:
		HandleGateServerShutdown(base);
		break;
	case ServerType::DatabaseServer:
		HandleDatabaseServerShutdown(base);
		break;
	case ServerType::LogicServer:
		HandleLogicServerShutdown(base);
		break;
	}

	return 0;
}

int RegClientReconnectFunc(std::function<void(const char*, int)> func)
{
	SetClientReconnectFunc(func);
	return 0;
}