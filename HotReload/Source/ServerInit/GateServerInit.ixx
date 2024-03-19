module;
#include "StdAfx.h"
#include "google/protobuf/message.h"
#include "hv/Channel.h"

#include <functional>
export module GateServerInit;

import DNServer;
import GateServer;
import GateServerHelper;
import MessagePack;
import GateMessage;
import Entity;

using namespace hv;
using namespace std;
using namespace google::protobuf;

export int HandleGateServerInit(DNServer *server);
export int HandleGateServerShutdown(DNServer *server);



int HandleGateServerInit(DNServer *server)
{
	SetGateServer(static_cast<GateServer*>(server));

	GateMessageHandle::RegMsgHandle();

	GateServerHelper* serverProxy = GetGateServer();

	if (auto serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;

		auto onConnection = [serverProxy](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();
			if (channel->isConnected())
			{
				DNPrint(2, LoggerLevel::Debug, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
			}
			else
			{
				DNPrint(3, LoggerLevel::Debug, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
				if(Entity* entity = channel->getContext<Entity>())
				{
					entity->CloseEvent()(entity);
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
				GateMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else
			{
				DNPrint(12, LoggerLevel::Error, nullptr);
			}
		};

		serverSock->onConnection = onConnection;
		serverSock->onMessage = onMessage;
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
				DNPrint(4, LoggerLevel::Debug, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
				channel->setHeartbeat(4000, std::bind(&DNClientProxyHelper::TickHeartbeat, clientSock));
				channel->setWriteTimeout(12000);
			}
			else
			{
				DNPrint(5, LoggerLevel::Debug, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
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

				if(DNTask<Message *>* task = clientSock->GetMsg(packet.msgId)) //client sock request
				{
					clientSock->DelMsg(packet.msgId);
					task->Resume();
					Message* message = task->GetResult();
					message->ParseFromArray((const char*)buf->data() + MessagePacket::PackLenth, packet.pkgLenth);
					task->CallResume();
				}
				else
				{
					DNPrint(13, LoggerLevel::Error, nullptr);
				}
			}
			else if(packet.dealType == MsgDeal::Req)
			{
				string msgData((char*)buf->data() + MessagePacket::PackLenth, packet.pkgLenth);
				GateMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else
			{
				DNPrint(12, LoggerLevel::Error, nullptr);
			}
		};

		clientSock->onConnection = onConnection;
		clientSock->onMessage = onMessage;
		clientSock->SetRegistEvent(&Evt_ReqRegistSrv);
	}

	return true;
}

int HandleGateServerShutdown(DNServer *server)
{
	GateServerHelper* serverProxy = GetGateServer();

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

	return true;
}