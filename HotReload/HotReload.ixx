
#include "hv/Channel.h"
#include "hv/hloop.h"
#include "schema.pb.h"

import DimensionNightmare;

#ifdef HOTRELOAD_BUILD
#define HOTRELOAD __declspec(dllexport)
#else
#define HOTRELOAD __declspec(dllimport)
#endif

using namespace hv;
using namespace std;

#ifdef __cplusplus
extern "C"
{
#endif

    HOTRELOAD int InitHotReload(DimensionNightmare &server);
    HOTRELOAD int ShutdownHotReload(DimensionNightmare &server);

#ifdef __cplusplus
}
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

int InitHotReload(DimensionNightmare &nightmare)
{
    nightmare.GetServer()->onConnection = [&nightmare](const SocketChannelPtr &channel)
    {
        string peeraddr = channel->peeraddr();
        if (channel->isConnected())
        {
            static int entityId = 0;
            nightmare.GetActorManager()->AddEntity(channel.get(), ++entityId);

            printf("%s connected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
        }
        else
        {
            nightmare.GetActorManager()->RemoveEntity(channel.get());
            printf("%s disconnected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
        }
    };

    nightmare.GetServer()->onMessage = [&nightmare](const SocketChannelPtr &channel, Buffer *buf) 
    {
        
    };

    return 0;
}

int ShutdownHotReload(DimensionNightmare &nightmare)
{
    nightmare.GetServer()->onConnection = nullptr;
    nightmare.GetServer()->onMessage = nullptr;
    return 0;
}