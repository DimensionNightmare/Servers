
module;

export module HotReload;

import DNServer;
import GlobalServerInit;
import ControlServerInit;
import AuthServerInit;
import GateServerInit;
import DatabaseServerInit;
import LogicServerInit;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ThirdParty.Platform;
import ECSW;

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

	HOTRELOAD int InitHotReload(World* world)
	{
		Libhv::hvlog_disable();

		DNServer::Ptr dnServer = world->GetSystem<DNServer>(EMSystemType::DNServer);
		
		bool isDeal = false;
		
		switch (dnServer->GetServerType())
		{
			case EMServerType::ControlServer:
				isDeal = HandleControlServerInit(dnServer);
				break;
			case EMServerType::GlobalServer:
				isDeal = HandleGlobalServerInit(dnServer);
				break;
			case EMServerType::AuthServer:
				isDeal = HandleAuthServerInit(dnServer);
				break;
			case EMServerType::GateServer:
				isDeal = HandleGateServerInit(dnServer);
				break;
			case EMServerType::DatabaseServer:
				isDeal = HandleDatabaseServerInit(dnServer);
				break;
			case EMServerType::LogicServer:
				isDeal = HandleLogicServerInit(dnServer);
				break;
			default:
				break;
		}

		return isDeal;
	}

	HOTRELOAD int ShutdownHotReload(World* world)
	{
		DNServer::Ptr dnServer = world->GetSystem<DNServer>(EMSystemType::DNServer);

		bool isDeal = false;
		switch (servertype)
		{
			case EMServerType::ControlServer:
				isDeal = HandleControlServerShutdown(dnServer);
				break;
			case EMServerType::GlobalServer:
				isDeal = HandleGlobalServerShutdown(dnServer);
				break;
			case EMServerType::AuthServer:
				isDeal = HandleAuthServerShutdown(dnServer);
				break;
			case EMServerType::GateServer:
				isDeal = HandleGateServerShutdown(dnServer);
				break;
			case EMServerType::DatabaseServer:
				isDeal = HandleDatabaseServerShutdown(dnServer);
				break;
			case EMServerType::LogicServer:
				isDeal = HandleLogicServerShutdown(dnServer);
				break;
			default:
				break;
		}

		ShutdownProtobufLibrary();
		Libhv::cleanup();

		return isDeal;
	}

}
