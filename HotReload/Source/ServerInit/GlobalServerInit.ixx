module;
#include "google/protobuf/dynamic_message.h"

#include "hv/Channel.h"
#include "hv/hloop.h"
export module GlobalServerInit;

import GlobalMessage;
import MessagePack;
import GlobalServer;
import GlobalServerHelper;

using namespace hv;
using namespace std;
using namespace google::protobuf;

export void HandleGlobalServerInit(GlobalServer *server);
export void HandleGlobalServerShutdown(GlobalServer *server);

module:private;

void HandleGlobalServerInit(GlobalServer *server)
{
	SetGlobalServer(server);

	if (auto sSock = server->GetSSock())
	{
		auto onConnection = [&](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();
			if (channel->isConnected())
			{
				printf("%s-> %s connected! connfd=%d id=%d \n", __FUNCTION__, peeraddr.c_str(), channel->fd(), channel->id());
			}
			else
			{
				printf("%s-> %s disconnected! connfd=%d id=%d \n", __FUNCTION__, peeraddr.c_str(), channel->fd(), channel->id());
			}
		};

		auto onMessage = [](const SocketChannelPtr &channel, Buffer *buf) 
		{
			
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
				printf("%s-> %s connected! connfd=%d id=%d \n", __FUNCTION__, peeraddr.c_str(), channel->fd(), channel->id());

				// send RegistInfo
				// globalSrv->RegistSelf<GlobalServerHelper*>(&Msg_RegistSrv);
				auto cSock = globalSrv->GetCSock();
				auto loop = cSock->loop();
				loop->setInterval(1000, [globalSrv](TimerID timerID)
				{
					auto cSock = globalSrv->GetCSock();
					auto loop = cSock->loop();

					if (cSock->channel->isConnected() && !cSock->IsRegisted()) 
					{
						Msg_RegistSrv(globalSrv);
					} 
					else 
					{
						loop->killTimer(timerID);
					}
				});
			}
			else
			{
				printf("%s-> %s disconnected! connfd=%d id=%d \n", __FUNCTION__, peeraddr.c_str(), channel->fd(), channel->id());
				globalSrv->GetCSock()->SetRegisted(false);
			}

			if(globalSrv->GetCSock()->isReconnect())
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
				MessagePacket packet;
				memcpy(&packet, buf->data(), MessagePacket::PackLenth);
				if(packet.dealType == MsgDeal::Req)
				{
					string msgData((char*)buf->data() + MessagePacket::PackLenth, packet.pkgLenth);
					GlobalMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
			}
		};

		cSock->onConnection = onConnection;
		cSock->onMessage = onMessage;
		
	}

	// regist self if need
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