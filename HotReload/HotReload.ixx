
#include "hv/Channel.h"
#include "hv/hloop.h"
#include "schema.pb.h"
#include "google/protobuf/any.pb.h"

#include <codecvt>

import BaseServer;

#ifdef HOTRELOAD_BUILD
#define HOTRELOAD __declspec(dllexport)
#else
#define HOTRELOAD __declspec(dllimport)
#endif

using namespace hv;
using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

HOTRELOAD int ServerInit(BaseServer&  server);
HOTRELOAD int ServerUnload(BaseServer&  server);

#ifdef __cplusplus
}
#endif

BOOL WINAPI DllMain(HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpvReserved )  // reserved
{
    // Perform actions based on the reason for calling.
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
         // Initialize once for each new process.
         // Return FALSE to fail DLL load.
            break;

        case DLL_THREAD_ATTACH:
         // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
         	google::protobuf::ShutdownProtobufLibrary();
            break;

        case DLL_PROCESS_DETACH:
        
            if (lpvReserved != nullptr)
            {
                break; // do not do cleanup if process termination scenario
            }
            
         // Perform any necessary cleanup.
            break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

int ServerInit(BaseServer&  server)
{
    static int headLen = 0;
    if(server.unpack_setting)
    {
        headLen = server.unpack_setting->body_offset;
    }

    server.onConnection = [](const SocketChannelPtr& channel) 
    {
        string peeraddr = channel->peeraddr();
        if (channel->isConnected()) {
            printf("%s connected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
        } else {
            printf("%s disconnected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
        }
    };

    server.onMessage = [](const SocketChannelPtr& channel, Buffer* buf) 
    {
        int msgSize = (int)buf->size() - headLen;
        assert(msgSize > 0);
        unsigned char* pData = (unsigned char*)buf->data() + headLen;

        google::protobuf::Any msg;
        if(msg.ParseFromArray(pData, msgSize))
        {
            if(msg.Is<GCfg::WeaponInfo>())
            {
                GCfg::WeaponInfo weaponInfo;
                msg.UnpackTo(&weaponInfo);
                static wstring_convert<codecvt_utf8<wchar_t>> converter;
                wstring wideString = converter.from_bytes(weaponInfo.Utf8DebugString());
                wcout << wideString << endl;
            }
        }
        else
            printf("%d < %.*s\n", (int)buf->size(),(int)buf->size(), (char*)buf->data());
        
        channel->write(buf);
    };

    return 0;
}

int ServerUnload(BaseServer&  server)
{
    server.onConnection = nullptr;
    server.onMessage = nullptr;
    return 0;
}