module;
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
		HandleControlServerInit(&base);
		break;
	}
	case ServerType::GlobalServer:
	{
		HandleGlobalServerInit(&base);
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
			HandleControlServerShutdown(&base);
		}
		break;

	case ServerType::GlobalServer:
		{
			HandleGlobalServerShutdown(&base);
		}
		break;
	}

	return 0;
}