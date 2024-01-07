module;
#include "google/protobuf/Message.h"
#include "hv/Channel.h"

#include <functional>
export module GlobalServerInit;

import DNServer;
import GlobalServer;
import GlobalServerHelper;
import MessagePack;
import GlobalMessage;

using namespace hv;
using namespace std;
using namespace google::protobuf;

export void HandleGlobalServerInit(DNServer *server);
export void HandleGlobalServerShutdown(DNServer *server);

module:private;

void HandleGlobalServerInit(DNServer *server)
{
	SetGlobalServer(static_cast<GlobalServer*>(server));

	auto serverProxy = GetGlobalServer();

	if (auto sSock = serverProxy->GetSSock())
	{
		auto onConnection = [&](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();
			if (channel->isConnected())
			{
				printf("%s->%s connected! connfd=%d id=%d \n", __FUNCTION__, peeraddr.c_str(), channel->fd(), channel->id());
			}
			else
			{
				printf("%s->%s disconnected! connfd=%d id=%d \n", __FUNCTION__, peeraddr.c_str(), channel->fd(), channel->id());
			}
		};

		auto onMessage = [](const SocketChannelPtr &channel, Buffer *buf) 
		{
			
		};

		sSock->onConnection = onConnection;
		sSock->onMessage = onMessage;
	}

	if (auto cSock = serverProxy->GetCSock())
	{
		auto onConnection = [](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();
			auto cProxy = GetGlobalServer()->GetCSock();

			if (channel->isConnected())
			{
				printf("%s->%s connected! connfd=%d id=%d \n", __FUNCTION__, peeraddr.c_str(), channel->fd(), channel->id());
				channel->onclose = std::bind(&DNClientProxyHelper::ServerDisconnect, cProxy);
				// send RegistInfo
				cProxy->StartRegist();
			}
			else
			{
				printf("%s->%s disconnected! connfd=%d id=%d \n", __FUNCTION__, peeraddr.c_str(), channel->fd(), channel->id());
			}

			if(cProxy->isReconnect())
			{
				
			}
		};

		auto onMessage = [](const SocketChannelPtr &channel, Buffer *buf) 
		{
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.dealType == MsgDeal::Res)
			{
				auto& reqMap = GetGlobalServer()->GetCSock()->GetMsgMap();
				if(reqMap.contains(packet.msgId)) //client sock request
				{
					auto task = reqMap.at(packet.msgId);
					reqMap.erase(packet.msgId);
					task->Resume();
					Message* message = ((DNTask<Message*>*)task)->GetResult();
					message->ParseFromArray((const char*)buf->data() + MessagePacket::PackLenth, packet.pkgLenth);
					task->CallResume();
				}
				else
				{
					fprintf(stderr, "%s->cant find msgid! \n", __FUNCTION__);
				}
			}
			else if(packet.dealType == MsgDeal::Req)
			{
				string msgData((char*)buf->data() + MessagePacket::PackLenth, packet.pkgLenth);
				GlobalMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else
			{
				fprintf(stderr, "%s->packet.dealType not matching! \n", __FUNCTION__);
			}
		};

		cSock->onConnection = onConnection;
		cSock->onMessage = onMessage;
		cSock->SetRegistEvent(&Msg_RegistSrv);
	}

}

void HandleGlobalServerShutdown(DNServer *server)
{
	auto serverProxy = GetGlobalServer();

	if (auto sSock = serverProxy->GetSSock())
	{
		sSock->onConnection = nullptr;
		sSock->onMessage = nullptr;
	}

	if (auto cSock = serverProxy->GetCSock())
	{
		cSock->onConnection = nullptr;
		cSock->onMessage = nullptr;
		cSock->SetRegistEvent(nullptr);
	}
}