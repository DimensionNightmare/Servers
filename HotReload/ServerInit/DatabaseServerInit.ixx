module;
#include "StdAfx.h"

#include "google/protobuf/message.h"
#include "hv/Channel.h"
#include <functional>
#include <format>
export module DatabaseServerInit;

import DNServer;
import DatabaseServer;
import DatabaseServerHelper;
import MessagePack;
import DatabaseMessage;

using namespace hv;
using namespace std;
using namespace google::protobuf;

export int HandleDatabaseServerInit(DNServer *server);
export int HandleDatabaseServerShutdown(DNServer *server);

int HandleDatabaseServerInit(DNServer *server)
{
	SetDatabaseServer(static_cast<DatabaseServer*>(server));

	DatabaseMessageHandle::RegMsgHandle();
	
	DatabaseServerHelper* serverProxy = GetDatabaseServer();

	if (DNClientProxyHelper* clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;
		
		auto onConnection = [serverProxy](const SocketChannelPtr &channel)
		{
			DNClientProxyHelper* clientSock = serverProxy->GetCSock();

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

		auto onMessage = [serverProxy](const SocketChannelPtr &channel, Buffer *buf) 
		{
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.dealType == MsgDeal::Req)
			{
				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
				DatabaseMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else if(packet.dealType == MsgDeal::Ret)
			{
				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
				DatabaseMessageHandle::MsgRetHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else if(packet.dealType == MsgDeal::Res)
			{
				DNClientProxyHelper* clientSock = serverProxy->GetCSock();

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

	return serverProxy->InitDatabase();
}

int HandleDatabaseServerShutdown(DNServer *server)
{
	DatabaseServerHelper* serverProxy = GetDatabaseServer();

	if (DNClientProxyHelper* clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;
		clientSock->SetRegistEvent(nullptr);
	}

	return true;
}