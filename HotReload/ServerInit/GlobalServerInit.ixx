module;
#include "hv/Channel.h"
#include "hv/hloop.h"

#include "Common.pb.h"
export module GlobalServerInit;

import GlobalServer;
import GlobalMessage;

using namespace hv;
using namespace std;
using namespace google::protobuf;

export void HandleGlobalServerInit(GlobalServer *server);
export void HandleGlobalServerShutdown(GlobalServer *server);

module:private;

void HandleGlobalServerInit(GlobalServer *server)
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
		auto onConnection = [&cSock, &server](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();
			if (channel->isConnected())
			{
				printf("%s connected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());

				// send RegistInfo
				Msg_RegistSelf((int)ServerType::GlobalServer, *server->GetSSock());
			}
			else
			{
				printf("%s disconnected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
			}

			if(cSock->isReconnect())
			{

			}
		};
		auto onMessage = [](const SocketChannelPtr &channel, Buffer *buf) {

		};

		cSock->onConnection = onConnection;
		cSock->onMessage = onMessage;
	}
}

void HandleGlobalServerShutdown(GlobalServer *server)
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