module;
#include <functional>
#include <format>
#include "google/protobuf/message.h"
#include "hv/Channel.h"

#include "StdAfx.h"
export module DatabaseServerInit;

import DatabaseServerHelper;
import MessagePack;
import DatabaseMessage;
import DNTask;

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
		
		auto onConnection = [clientSock, serverProxy](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();

			if (channel->isConnected())
			{
				DNPrint(TipCode_SrvConnOn, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
				clientSock->SetRegistEvent(&DatabaseMessage::Evt_ReqRegistSrv);	
			}
			else
			{
				DNPrint(TipCode_SrvConnOff, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
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
				DatabaseMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else if(packet.dealType == MsgDeal::Ret)
			{
				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
				DatabaseMessageHandle::MsgRetHandle(channel, packet.msgId, packet.msgHashId, msgData);
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
					DNPrint(ErrCode_MsgFind, LoggerLevel::Error, nullptr);
				}
			}
			else
			{
				DNPrint(ErrCode_MsgDealType, LoggerLevel::Error, nullptr);
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