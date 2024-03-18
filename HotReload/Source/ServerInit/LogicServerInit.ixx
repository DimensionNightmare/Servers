module;
#include "StdAfx.h"
#include "google/protobuf/message.h"
#include "hv/Channel.h"

#include <functional>
#include <format>
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
	
	if (auto clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;
		
		auto onConnection = [serverProxy](const SocketChannelPtr &channel)
		{
			auto clientSock = serverProxy->GetCSock();

			string peeraddr = channel->peeraddr();

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
							GetClientReconnectFunc()(serverProxy->GetCtlIp().c_str(), serverProxy->GetCtlPort());
						}
					}
				}
			}
			

			if (channel->isConnected())
			{
				DNPrint(4, LoggerLevel::Debug, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
				clientSock->SetRegistEvent(&Msg_RegistSrv);
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
			if(packet.dealType == MsgDeal::Req)
			{
				string msgData((char*)buf->data() + MessagePacket::PackLenth, packet.pkgLenth);
				LogicMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else if(packet.dealType == MsgDeal::Ret)
			{
				string msgData((char*)buf->data() + MessagePacket::PackLenth, packet.pkgLenth);
				LogicMessageHandle::MsgRetHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else if(packet.dealType == MsgDeal::Res)
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
			else
			{
				DNPrint(12, LoggerLevel::Error, nullptr);
			}
		};

		clientSock->onConnection = onConnection;
		clientSock->onMessage = onMessage;
		clientSock->SetRegistEvent(&Msg_RegistSrv);
	}

	return true;
}

int HandleLogicServerShutdown(DNServer *server)
{
	LogicServerHelper* serverProxy = GetLogicServer();

	if (auto clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;
		clientSock->SetRegistEvent(nullptr);
	}

	return true;
}