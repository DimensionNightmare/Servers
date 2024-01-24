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
import ServerEntityHelper;

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

	if (auto serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;
		
		auto onConnection = [serverProxy](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();
			if (channel->isConnected())
			{
				DNPrint("%s connected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
			}
			else
			{
				DNPrint("%s disconnected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());

				if(auto entity = channel->getContext<ServerEntityHelper>())
				{
					auto entityMan = serverProxy->GetEntityManager();
					entityMan->RemoveEntity(entity->GetChild()->GetID());
				}
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

		serverSock->onConnection = onConnection;
		serverSock->onMessage = onMessage;

		GlobalMessageHandle::RegMsgHandle();
	}

	if (auto clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;

		auto onConnection = [serverProxy](const SocketChannelPtr &channel)
		{
			auto clientSock = serverProxy->GetCSock();

			string peeraddr = channel->peeraddr();
			clientSock->UpdateClientState(channel->status);

			if (channel->isConnected())
			{
				DNPrint("%s connected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
			}
			else
			{
				DNPrint("%s disconnected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
			}

			if(clientSock->isReconnect())
			{
				
			}
		};

		auto onMessage = [serverProxy](const SocketChannelPtr &channel, Buffer *buf) 
		{
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.dealType == MsgDeal::Res)
			{
				auto clientSock = serverProxy->GetCSock();

				if(auto task = clientSock->GetMsg(packet.msgId)) //client sock request
				{
					clientSock->DelMsg(packet.msgId);
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

		clientSock->onConnection = onConnection;
		clientSock->onMessage = onMessage;
		clientSock->SetRegistEvent(&Msg_RegistSrv);
	}

}

void HandleGlobalServerShutdown(DNServer *server)
{
	auto serverProxy = GetGlobalServer();

	if (auto serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;
	}

	if (auto clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;
		clientSock->SetRegistEvent(nullptr);
	}
}