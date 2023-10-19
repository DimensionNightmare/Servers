module;
#include "hv/Channel.h"
#include "hv/hloop.h"

export module ControlServerInit;

import ControlServer;

using namespace hv;
using namespace std;

export void HandleControlServerInit(ControlServer *server);
export void HandleControlServerShutdown(ControlServer *server);

module:private;

void HandleControlServerInit(ControlServer *server)
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
			auto data = buf->data();
		};

		sSock->onConnection = onConnection;
		sSock->onMessage = onMessage;
	}
}

void HandleControlServerShutdown(ControlServer *server)
{
	if (auto sSock = server->GetSSock())
	{
		sSock->onConnection = nullptr;
		sSock->onMessage = nullptr;
	}
}