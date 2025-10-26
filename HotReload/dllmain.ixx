
export module DLLMAIN;

import GlobalServerHelper;
import ControlServerHelper;
import AuthServerHelper;
import GateServerHelper;
import DatabaseServerHelper;
import LogicServerHelper;
import ThirdParty.Platform;
import ECSW;
import Server;
import ThirdParty.Libhv;
import std.compat;
import ThirdParty.Protobuf;
import HotReload;

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


int InitHotReload(World::Ptr world)
{
	Libhv::hvlog_disable();

	Server::Ptr dnServer = world->GetSystem<Server>(EMSystemType::Server);

	switch (dnServer->GetServerType())
	{
		#define one(Type) case EMServerType::Type:{ return dnServer->GetSelf<Type##Helper>()->HandleServerInit(); }
		one(ControlServer)
		one(GlobalServer)
		one(AuthServer)
		one(GateServer)
		one(DatabaseServer)
		one(LogicServer)

		#undef one
	}

	return 0;
}

int ShutdownHotReload(World::Ptr world)
{
	Server::Ptr dnServer = world->GetSystem<Server>(EMSystemType::Server);

	switch (dnServer->GetServerType())
	{
		#define one(Type) case EMServerType::Type: { return dnServer->GetSelf<Type##Helper>()->HandleServerShutdown();}
		one(ControlServer)
		one(GlobalServer)
		one(AuthServer)
		one(GateServer)
		one(DatabaseServer)
		one(LogicServer)
		#undef one
	}

	return 0;
}

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
			{
				// lpvReserved != nullptr ; Staticly linked DLL process detach
				// lpvReserved == nullptr ; LoadLibrary Dynamically linked DLL process detach 

				ShutdownProtobufLibrary();
				Libhv::cleanup();
				break;
			}
			// DLL_PROCESS_ATTACH
			case 1:
			{
				using funcSign = void (*)(InstanceHolder::Ptr&);
				auto funtPtr = Platform::GetFuncPtr(nullptr, "GetInstanceHolder");
				if (funcSign func = reinterpret_cast<funcSign>(funtPtr))
				{
					func(P_InstanceHolder);
				
					HotReload::Ptr pHotDll = P_InstanceHolder->AuthWorld->GetSystem<HotReload>(EMSystemType::HotReload);
					{
						pHotDll->pInitHotReload = &InitHotReload;
					}
					{	
						pHotDll->pShutdownHotReload = &ShutdownHotReload;
					}
				}
				break;
			}
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

}
