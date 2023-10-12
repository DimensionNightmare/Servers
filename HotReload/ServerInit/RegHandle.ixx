module;
#include "hv/Channel.h"
#include "hv/hloop.h"
export module RegHandle;

import BaseServer;
import ControlServer;
import GlobalServer;
import SessionServer;

import GlobalServerInit;
import ControlServerInit;
import SessionServerInit;

#ifdef HOTRELOAD_BUILD
#define HOTRELOAD __declspec(dllexport)
#else
#define HOTRELOAD __declspec(dllimport)
#endif

extern "C"
{
	HOTRELOAD int InitHotReload(BaseServer &base);
	HOTRELOAD int ShutdownHotReload(BaseServer &base);
}

module:private;

int InitHotReload(BaseServer &base)
{
	ServerType servertype = base.GetServerType();
	switch (servertype)
	{
	case ServerType::ControlServer:
	{
		ControlServer *server = (ControlServer *)&base;
		HandleControlServerInit(server);
		break;
	}
	case ServerType::GlobalServer:
	{
		GlobalServer *server = (GlobalServer *)&base;
		HandleGlobalServerInit(server);
		break;
	}
	case ServerType::SessionServer:
	{

		SessionServer *server = (SessionServer *)&base;
		HandleSessionServerInit(server);
		break;
	}
	}

	return 0;
}

int ShutdownHotReload(BaseServer &base)
{
	ServerType servertype = base.GetServerType();
	switch (servertype)
	{
	case ServerType::ControlServer:
	{
		ControlServer *server = (ControlServer *)&base;
		HandleControlServerShutdown(server);
	}

	break;
	case ServerType::GlobalServer:
	{

		GlobalServer *server = (GlobalServer *)&base;
		HandleGlobalServerShutdown(server);
	}
	break;
	case ServerType::SessionServer:
	{
		SessionServer *server = (SessionServer *)&base;
		HandleSessionServerShutdown(server);
	}

	break;
	}

	return 0;
}