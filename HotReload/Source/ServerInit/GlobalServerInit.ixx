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
import AfxCommon;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);

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
		auto onConnection = [sSock,serverProxy](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();
			if (channel->isConnected())
			{
				DNPrint("%s connected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
			}
			else
			{
				DNPrint("%s disconnected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());

				auto entityMan = serverProxy->GetEntityManager();
				entityMan->RemoveEntity<ServerEntityHelper>(channel, false);
			}
		};

		auto onMessage = [](const SocketChannelPtr &channel, Buffer *buf) 
		{
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.dealType == MsgDeal::Req)
			{
				string msgData((char*)buf->data() + MessagePacket::PackLenth, packet.pkgLenth);
				GlobalMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else
			{
				DNPrintErr("packet.dealType not matching! \n");
			}
		};

		sSock->onConnection = onConnection;
		sSock->onMessage = onMessage;

		GlobalMessageHandle::RegMsgHandle();
	}

	if (auto cSock = serverProxy->GetCSock())
	{
		auto onConnection = [cSock](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();
			cSock->UpdateClientState(channel->status);

			if (channel->isConnected())
			{
				DNPrint("%s connected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
			}
			else
			{
				DNPrint("%s disconnected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
			}

			if(cSock->isReconnect())
			{
				
			}
		};

		auto onMessage = [cSock](const SocketChannelPtr &channel, Buffer *buf) 
		{
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.dealType == MsgDeal::Res)
			{
				if(auto task = cSock->GetMsg(packet.msgId)) //client sock request
				{
					cSock->DelMsg(packet.msgId);
					task->Resume();
					Message* message = ((DNTask<Message*>*)task)->GetResult();
					message->ParseFromArray((const char*)buf->data() + MessagePacket::PackLenth, packet.pkgLenth);
					task->CallResume();
				}
				else
				{
					DNPrintErr("cant find msgid! \n");
				}
			}
			else if(packet.dealType == MsgDeal::Req)
			{
				string msgData((char*)buf->data() + MessagePacket::PackLenth, packet.pkgLenth);
				GlobalMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else
			{
				DNPrintErr("packet.dealType not matching! \n");
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