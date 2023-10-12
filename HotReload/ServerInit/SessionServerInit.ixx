module;
#include "hv/Channel.h"
#include "hv/hloop.h"

export module SessionServerInit;

import SessionServer;

using namespace hv;
using namespace std;

export void HandleSessionServerInit(SessionServer *server);
export void HandleSessionServerShutdown(SessionServer *server);

module:private;

void HandleSessionServerInit(SessionServer *server)
{
	if (auto sSock = server->GetSSock())
	{
		auto onConnection = [&](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();
			if (channel->isConnected())
			{
				printf("%s connected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
			}
			else
			{
				printf("%s disconnected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
			}
		};

		auto onMessage = [](const SocketChannelPtr &channel, Buffer *buf) {

		};

		sSock->onConnection = onConnection;
		sSock->onMessage = onMessage;
	}

	if (auto cSock = server->GetCSock())
	{
		auto onConnection = [](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();
			if (channel->isConnected())
			{
				printf("%s connected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());

				// send RegistInfo
			}
			else
			{
				printf("%s disconnected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
			}
		};
		auto onMessage = [](const SocketChannelPtr &channel, Buffer *buf) {

		};

		cSock->onConnection = onConnection;
		cSock->onMessage = onMessage;
	}
}

void HandleSessionServerShutdown(SessionServer *server)
{
	if (auto sSock = server->GetSSock())
	{
		sSock->onConnection = nullptr;
		sSock->onMessage = nullptr;
	}

	if (auto cSock = server->GetCSock())
	{
		cSock->onConnection = nullptr;
		cSock->onMessage = nullptr;
	}
}