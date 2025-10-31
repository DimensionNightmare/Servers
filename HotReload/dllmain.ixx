
export module DLLMAIN;

import ThirdParty.Platform;
import ECSW;
import ThirdParty.Libhv;
import std.compat;
import ThirdParty.Protobuf;
import DimensionNightmareHelper;

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
				Libhv::hvlog_disable();
				
				using funcSign = void (*)(InstanceHolder::Ptr&);
				auto funtPtr = Platform::GetFuncPtr(nullptr, "GetInstanceHolder");
				if (funcSign func = reinterpret_cast<funcSign>(funtPtr))
				{
					func(P_InstanceHolder);
				
					DimensionNightmareHelper::Ptr MainWorld = P_InstanceHolder->MainWorld->GetSelf<DimensionNightmareHelper>();
					MainWorld->InitHotReload();
					MainWorld->DeInitHotReload();
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
