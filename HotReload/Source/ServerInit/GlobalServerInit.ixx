module;
#include "hv/Channel.h"
#include "hv/hloop.h"

#include "GlobalControl.pb.h"
export module GlobalServerInit;

export import GlobalMessage;
import DNTask;
import MessagePack;

using namespace hv;
using namespace std;
using namespace google::protobuf;

export void HandleGlobalServerInit(GlobalServer *server);
export void HandleGlobalServerShutdown(GlobalServer *server);


module:private;

void HandleGlobalServerInit(GlobalServer *server)
{
	GetGlobalServer(server);

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
			auto globalSrv = GetGlobalServer();
			if (channel->isConnected())
			{
				printf("%s connected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());

				// send RegistInfo
				
				Msg_RegistSrv((int)ServerType::GlobalServer, *globalSrv->GetCSock(), *globalSrv->GetSSock());
			}
			else
			{
				printf("%s disconnected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
			}

			if(globalSrv->GetCSock()->isReconnect())
			{

			}
		};
		auto onMessage = [](const SocketChannelPtr &channel, Buffer *buf) {
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.opType == MsgDir::Inner)
			{

				if(packet.msgId) //client sock request
				{
					auto reqMap = GetGlobalServer()->GetCSock()->GetMsgMap();
					if(reqMap->contains(packet.msgId)) 
					{
						auto pair = (*reqMap)[packet.msgId];
						reqMap->erase(packet.msgId);
						
						pair.second.Resume();
						Message* message = pair.second.GetResult();
						message->ParseFromArray((const char*)buf->data() + MessagePacket::PackLenth + packet.msgLenth, packet.pkgLenth - packet.msgLenth);
						pair.first.Resume();
					}
				}
			}
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