module;
#include "hv/Channel.h"
#include "hv/hloop.h"
export module RegHandle;

import DNServer;
import GlobalServerInit;
import ControlServerInit;

#ifdef HOTRELOAD_BUILD
#define HOTRELOAD __declspec(dllexport)
#else
#define HOTRELOAD __declspec(dllimport)
#endif

extern "C"
{
	HOTRELOAD int InitHotReload(DNServer &base);
	HOTRELOAD int ShutdownHotReload(DNServer &base);
}

module:private;

int InitHotReload(DNServer &base)
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
		ControlServer *server = (ControlServer *)&base;
		HandleControlServerShutdown(server);
	break;
	}

	case ServerType::GlobalServer:
	{

		GlobalServer *server = (GlobalServer *)&base;
		HandleGlobalServerShutdown(server);
	break;
	}
	}

	return 0;
}