
module;

export module HotReload;

import GlobalServerInit;
import ControlServerInit;
import AuthServerInit;
import GateServerInit;
import DatabaseServerInit;
import LogicServerInit;
import ThirdParty.Platform;
import ECSW;
import DNServer;
import ThirdParty.Libhv;
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
	int DllMain(Platform::HotHandle hinstDLL, uint32_t fdwReason, void* lpvReserved)
	{
		// Perform actions based on the reason for calling.
		switch (fdwReason)
		{
			// DLL_PROCESS_DETACH
			case 0:
				// lpvReserved != nullptr ; Staticly linked DLL process detach
				// lpvReserved == nullptr ; LoadLibrary Dynamically linked DLL process detach 
				
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

	HOTRELOAD int InitHotReload(const World::Ptr& world)
	{
		Libhv::hvlog_disable();

		DNServer::Ptr dnServer = world->GetSystem<DNServer>(EMSystemType::DNServer);
		
		bool isDeal = false;
		
		switch (dnServer->GetServerType())
		{
			case EMServerType::ControlServer:
				isDeal = HandleControlServerInit(world);
				break;
			case EMServerType::GlobalServer:
				isDeal = HandleGlobalServerInit(world);
				break;
			case EMServerType::AuthServer:
				isDeal = HandleAuthServerInit(world);
				break;
			case EMServerType::GateServer:
				isDeal = HandleGateServerInit(world);
				break;
			case EMServerType::DatabaseServer:
				isDeal = HandleDatabaseServerInit(world);
				break;
			case EMServerType::LogicServer:
				isDeal = HandleLogicServerInit(world);
				break;
			default:
				break;
		}

		return isDeal;
	}

	HOTRELOAD int ShutdownHotReload(const World::Ptr& world)
	{
		DNServer::Ptr dnServer = world->GetSystem<DNServer>(EMSystemType::DNServer);

		bool isDeal = false;
		switch (dnServer->GetServerType())
		{
			case EMServerType::ControlServer:
				isDeal = HandleControlServerShutdown(world);
				break;
			case EMServerType::GlobalServer:
				isDeal = HandleGlobalServerShutdown(world);
				break;
			case EMServerType::AuthServer:
				isDeal = HandleAuthServerShutdown(world);
				break;
			case EMServerType::GateServer:
				isDeal = HandleGateServerShutdown(world);
				break;
			case EMServerType::DatabaseServer:
				isDeal = HandleDatabaseServerShutdown(world);
				break;
			case EMServerType::LogicServer:
				isDeal = HandleLogicServerShutdown(world);
				break;
			default:
				break;
		}

		ShutdownProtobufLibrary();
		Libhv::cleanup();

		return isDeal;
	}

}
