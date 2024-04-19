module;
#include <functional>
#include <format>
#include "google/protobuf/message.h"
#include "hv/Channel.h"

#include "StdAfx.h"
export module LogicServerInit;

import DNServer;
import LogicServer;
import LogicServerHelper;
import MessagePack;
import LogicMessage;

using namespace hv;
using namespace std;
using namespace google::protobuf;

export int HandleLogicServerInit(DNServer *server);
export int HandleLogicServerShutdown(DNServer *server);

int HandleLogicServerInit(DNServer *server)
{
	SetLogicServer(static_cast<LogicServer*>(server));

	LogicMessageHandle::RegMsgHandle();
	
	LogicServerHelper* serverProxy = GetLogicServer();

	if (DNServerProxyHelper* serverSock = serverProxy->GetSSock())
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
				if(ServerEntityHelper* entity = channel->getContext<ServerEntityHelper>())
				{
					ServerEntityManagerHelper<ServerEntity>*  entityMan = serverProxy->GetServerEntityManager();
					entityMan->RemoveEntity(entity->ID());
					channel->setContext(nullptr);
				}
			}
		};

		auto onMessage = [](const SocketChannelPtr &channel, Buffer *buf) 
		{
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.dealType == MsgDeal::Req)
			{
				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
				LogicMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else if(packet.dealType == MsgDeal::Ret)
			{
				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
				LogicMessageHandle::MsgRetHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else
			{
				DNPrint(12, LoggerLevel::Error, nullptr);
			}
		};

		serverSock->onConnection = onConnection;
		serverSock->onMessage = onMessage;
	}


	if (DNClientProxyHelper* clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;
		
		//client will re_create please check
		auto onConnection = [clientSock,serverProxy](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();

			if (channel->isConnected())
			{
				DNPrint(4, LoggerLevel::Debug, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
				clientSock->SetRegistEvent(&Evt_ReqRegistSrv);
				
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

			ProxyStatus state = clientSock->UpdateClientState(channel->status);
			if(state != ProxyStatus::None)
			{
				switch(state)
				{
					case ProxyStatus::Close:
					{
						// not orgin
						string origin = format("{}:{}", serverProxy->GetCtlIp(), serverProxy->GetCtlPort());
						if(peeraddr != origin)
						{
							serverProxy->ReClientEvent(serverProxy->GetCtlIp(), serverProxy->GetCtlPort());
						}
					}
				}
			}
		};

		auto onMessage = [clientSock](const SocketChannelPtr &channel, Buffer *buf) 
		{
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.dealType == MsgDeal::Req)
			{
				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
				LogicMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else if(packet.dealType == MsgDeal::Ret)
			{
				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
				LogicMessageHandle::MsgRetHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else if(packet.dealType == MsgDeal::Res)
			{
				if(DNTask<Message>* task = clientSock->GetMsg(packet.msgId)) //client sock request
				{
					clientSock->DelMsg(packet.msgId);
					task->Resume();
					Message* message = task->GetResult();
					message->ParseFromArray(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
					task->CallResume();
				}
				else
				{
					DNPrint(13, LoggerLevel::Error, nullptr);
				}
			}
			else
			{
				DNPrint(12, LoggerLevel::Error, nullptr);
			}
		};

		clientSock->onConnection = onConnection;
		clientSock->onMessage = onMessage;
	}

	return true;
}

int HandleLogicServerShutdown(DNServer *server)
{
	LogicServerHelper* serverProxy = GetLogicServer();

	if (DNServerProxy* serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;
	}

	if (DNClientProxyHelper* clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;
		clientSock->SetRegistEvent(nullptr);
	}

	return true;
}