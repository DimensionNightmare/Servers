#include <iostream>
#include "hv/TcpServer.h"

#ifdef HOTRELOAD_BUILD
#define HOTRELOAD __declspec(dllexport)
#else
#define HOTRELOAD __declspec(dllimport)
#endif

using namespace hv;


#ifdef __cplusplus
extern "C" {
#endif

HOTRELOAD int ServerInit(TcpServer&  server);
HOTRELOAD int ServerUnload(TcpServer&  server);

#ifdef __cplusplus
}
#endif

int ServerInit(TcpServer&  server)
{
    server.onConnection = [](const SocketChannelPtr& channel) {
        std::string peeraddr = channel->peeraddr();
        if (channel->isConnected()) {
            printf("%s connected! connfdasdas=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
        } else {
            printf("%s disconnected! connfdasdsad=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
        }
    };
    server.onMessage = [](const SocketChannelPtr& channel, Buffer* buf) {
        // echo
        printf("< %.*s\n", (int)buf->size(), (char*)buf->data());
        channel->write(buf);
    };
    return 0;
}

int ServerUnload(TcpServer&  server)
{
    server.onConnection = nullptr;
    server.onMessage = nullptr;
    return 0;
}