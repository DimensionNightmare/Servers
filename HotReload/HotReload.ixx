
#include "google/protobuf/message.h"

#include <Windows.h>

#ifdef HOTRELOAD_BUILD
#define HOTRELOAD __declspec(dllexport)
#else
#define HOTRELOAD __declspec(dllimport)
#endif

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    // Perform actions based on the reason for calling.
    switch (fdwReason)
    {
    case DLL_PROCESS_DETACH:

        google::protobuf::ShutdownProtobufLibrary();
        if (lpvReserved != nullptr)
        {
            break; // do not do cleanup if process termination scenario
        }

        // Perform any necessary cleanup.
        break;
    case DLL_PROCESS_ATTACH:
        // Initialize once for each new process.
        // Return FALSE to fail DLL load.
        break;
    case DLL_THREAD_ATTACH:
        // Do thread-specific initialization.
        break;

    case DLL_THREAD_DETACH:

        break;
    }
    return TRUE; // Successful DLL_PROCESS_ATTACH.
}