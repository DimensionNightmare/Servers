
#include "StdAfx.h"
#include "google/protobuf/message.h"

#include <functional>
#include <string>

#ifdef _WIN32
	#include <Windows.h>
#endif

import DNServer;
import DimensionNightmare;
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
	HOTRELOAD int InitHotReload(DimensionNightmare &mainObj);
	HOTRELOAD int ShutdownHotReload(DimensionNightmare &mainObj);

	HOTRELOAD int RegClientReconnectFunc(std::function<void(const char*, unsigned short)> func);
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
		return HandleControlServerInit(base);
	case ServerType::GlobalServer:
		return HandleGlobalServerInit(base);
	case ServerType::AuthServer:
		return HandleAuthServerInit(base);
	case ServerType::GateServer:
		return HandleGateServerInit(base);
	case ServerType::DatabaseServer:
		return HandleDatabaseServerInit(base);
	case ServerType::LogicServer:
		return HandleLogicServerInit(base);
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
		return HandleControlServerShutdown(base);
	case ServerType::GlobalServer:
		return HandleGlobalServerShutdown(base);
	case ServerType::AuthServer:
		return HandleAuthServerShutdown(base);
	case ServerType::GateServer:
		return HandleGateServerShutdown(base);
	case ServerType::DatabaseServer:
		return HandleDatabaseServerShutdown(base);
	case ServerType::LogicServer:
		return HandleLogicServerShutdown(base);
	}

	return 0;
}

int RegClientReconnectFunc(std::function<void(const char*, unsigned short)> func)
{
	SetClientReconnectFunc(func);
	return 0;
}