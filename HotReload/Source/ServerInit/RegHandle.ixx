module;

#include <functional>
#include <string>
export module RegHandle;

import DNServer;
import GlobalServerInit;
import ControlServerInit;
import AuthServerInit;
import GateServerInit;
import DatabaseServerInit;
import LogicServerInit;

import DNClientProxy;
import DNClientProxyHelper;

#ifdef HOTRELOAD_BUILD
#define HOTRELOAD __declspec(dllexport)
#else
#define HOTRELOAD __declspec(dllimport)
#endif

extern "C"
{
	HOTRELOAD int InitHotReload(DNServer &base);
	HOTRELOAD int ShutdownHotReload(DNServer &base);

	HOTRELOAD int RegClientReconnectFunc(std::function<void(DNClientProxy *, const std::string&, int)> func);
}

module:private;

int InitHotReload(DNServer &base)
{
	ServerType servertype = base.GetServerType();
	switch (servertype)
	{
	case ServerType::ControlServer:
		{
			HandleControlServerInit(&base);
		}
		break;
	case ServerType::GlobalServer:
		{
			HandleGlobalServerInit(&base);
		}
		break;
	case ServerType::AuthServer:
		{
			HandleAuthServerInit(&base);
		}
		break;
	case ServerType::GateServer:
		{
			HandleGateServerInit(&base);
		}
		break;
	case ServerType::DatabaseServer:
		{
			HandleDatabaseServerInit(&base);
		}
		break;
	case ServerType::LogicServer:
		{
			HandleLogicServerInit(&base);
		}
		break;
	}

	return 0;
}

int ShutdownHotReload(DNServer &base)
{
	ServerType servertype = base.GetServerType();
	switch (servertype)
	{
	case ServerType::ControlServer:
		{
			HandleControlServerShutdown(&base);
		}
		break;
	case ServerType::GlobalServer:
		{
			HandleGlobalServerShutdown(&base);
		}
		break;
	case ServerType::AuthServer:
		{
			HandleAuthServerShutdown(&base);
		}
		break;
	case ServerType::GateServer:
		{
			HandleGateServerShutdown(&base);
		}
		break;
	case ServerType::DatabaseServer:
		{
			HandleDatabaseServerShutdown(&base);
		}
		break;
	case ServerType::LogicServer:
		{
			HandleLogicServerShutdown(&base);
		}
		break;
	}

	return 0;
}

int RegClientReconnectFunc(std::function<void(DNClientProxy *, const std::string&, int)> func)
{
	SetClientReconnectFunc(func);
	return 0;
}