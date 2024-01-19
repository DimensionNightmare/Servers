module;
#include "google/protobuf/Message.h"
#include "hv/Channel.h"

#include <functional>
export module DatabaseServerInit;

import DNServer;
import DatabaseServer;
import DatabaseServerHelper;
import MessagePack;
import DatabaseMessage;
import AfxCommon;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);

using namespace hv;
using namespace std;
using namespace google::protobuf;

export void HandleDatabaseServerInit(DNServer *server);
export void HandleDatabaseServerShutdown(DNServer *server);

module:private;

void HandleDatabaseServerInit(DNServer *server)
{
	SetDatabaseServer(static_cast<DatabaseServer*>(server));

	auto serverProxy = GetDatabaseServer();

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
				auto timerID = sSock->loop()->setTimeout(10000, [entityMan,channel](TimerID timerId)
				{
					entityMan->RemoveEntity<ServerEntityHelper>(channel);
				});

				if(auto entity = channel->getContext<ServerEntityHelper>())
				{
					auto child = entity->GetChild();
					child->SetTimerId(timerID);
					child->SetSock(nullptr);
				}
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
				DatabaseMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
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

void HandleDatabaseServerShutdown(DNServer *server)
{
	auto serverProxy = GetDatabaseServer();

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