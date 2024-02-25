module;
#include "StdAfx.h"
#include "google/protobuf/message.h"
#include "hv/Channel.h"

#include <functional>
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

module:private;

int HandleDatabaseServerInit(DNServer *server)
{
	SetDatabaseServer(static_cast<DatabaseServer*>(server));

	DatabaseMessageHandle::RegMsgHandle();
	
	DatabaseServerHelper* serverProxy = GetDatabaseServer();

	if (auto clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;
		
		auto onConnection = [serverProxy](const SocketChannelPtr &channel)
		{
			auto clientSock = serverProxy->GetCSock();

			string peeraddr = channel->peeraddr();
			
			if (channel->isConnected())
			{
				DNPrint(4, LoggerLevel::Debug, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
				clientSock->SetRegistEvent(&Msg_RegistSrv);
				clientSock->UpdateClientState(channel->status);
			}
			else
			{
				DNPrint(5, LoggerLevel::Debug, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
				clientSock->UpdateClientState(channel->status);
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
				DatabaseMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else
			{
				DNPrint(12, LoggerLevel::Error, nullptr);
			}
		};

		clientSock->onConnection = onConnection;
		clientSock->onMessage = onMessage;
	}

	return serverProxy->InitDabase();
}

int HandleDatabaseServerShutdown(DNServer *server)
{
	DatabaseServerHelper* serverProxy = GetDatabaseServer();

	if (auto clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;
		clientSock->SetRegistEvent(nullptr);
	}

	return true;
}