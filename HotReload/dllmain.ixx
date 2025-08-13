
export module DLLMAIN;

import GlobalServerHelper;
import ControlServerHelper;
import AuthServerHelper;
import GateServerHelper;
import DatabaseServerHelper;
import LogicServerHelper;
import GlobalServerMessage;
import ControlServerMessage;
import AuthServerMessage;
import GateServerMessage;
import DatabaseServerMessage;
import LogicServerMessage;
import ThirdParty.Platform;
import ECSW;
import Server;
import ThirdParty.Libhv;
import std.compat;
import ThirdParty.Protobuf;
import L10nText;
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


int InitHotReload(World::CVPtr world)
{
	Libhv::hvlog_disable();

	Server::CVPtr dnServer = world->GetSystem<Server>(EMSystemType::Server);

	L10nText::CVPtr dnL10n = world->GetSystem<L10nText>(EMSystemType::L10nText);
	

	switch (dnServer->GetServerType())
	{
		#define one(Type) case EMServerType::Type:{static Type##MessageHandle msgHandle; return dnServer->GetSelf<Type##Helper>()->HandleServerInit(&msgHandle); }
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

int ShutdownHotReload(World::CVPtr world)
{
	Server::CVPtr dnServer = world->GetSystem<Server>(EMSystemType::Server);

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

				using funcSign = World* (*)();
				auto funtPtr = Platform::GetFuncPtr(nullptr, "GetMainWorld");
				if (funcSign func = reinterpret_cast<funcSign>(funtPtr))
				{
					World* world = func();
					HotReload::CVPtr pHotDll = world->GetSystem<HotReload>(EMSystemType::HotReload);
					pHotDll->InitHotReload(std::function<int(World::CVPtr)>());
					pHotDll->ShutdownHotReload(std::function<int(World::CVPtr)>());
				}

				ShutdownProtobufLibrary();
				Libhv::cleanup();
				
				break;
			}
			// DLL_PROCESS_ATTACH
			case 1:
			{
				using funcSign = World* (*)();
				auto funtPtr = Platform::GetFuncPtr(nullptr, "GetMainWorld");
				if (funcSign func = reinterpret_cast<funcSign>(funtPtr))
				{
					World* world = func();
					HotReload::CVPtr pHotDll = world->GetSystem<HotReload>(EMSystemType::HotReload);
					pHotDll->InitHotReload(InitHotReload);
					pHotDll->ShutdownHotReload(ShutdownHotReload);
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
