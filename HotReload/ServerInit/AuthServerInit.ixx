module;
#include "google/protobuf/message.h"
#include "hv/Channel.h"

#include "StdAfx.h"
export module AuthServerInit;

import DNServer;
import AuthServer;
import AuthServerHelper;
import MessagePack;
import AuthMessage;

using namespace hv;
using namespace std;
using namespace google::protobuf;

export int HandleAuthServerInit(DNServer *server);
export int HandleAuthServerShutdown(DNServer *server);

int HandleAuthServerInit(DNServer *server)
{
	SetAuthServer(static_cast<AuthServer*>(server));

	AuthServerHelper* serverProxy = GetAuthServer();

	if(DNWebProxyHelper* serverSock = serverProxy->GetSSock())
	{
		HttpService* service = new HttpService;
		
		AuthMessageHandle::RegApiHandle(service);

		serverSock->registerHttpService(service);
	}

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
				DNPrint(2, LoggerLevel::Debug, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
				channel->setHeartbeat(4000, std::bind(&DNClientProxyHelper::TickHeartbeat, clientSock));
				channel->setWriteTimeout(12000);
				clientSock->SetRegistEvent(&Evt_ReqRegistSrv);
			}
			else
			{
				DNPrint(3, LoggerLevel::Debug, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
			}

			if(clientSock->isReconnect())
			{
				
			}

			clientSock->UpdateClientState(channel->status);
		};

		auto onMessage = [serverProxy](const SocketChannelPtr &channel, Buffer *buf) 
		{
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.dealType == MsgDeal::Res)
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

int HandleAuthServerShutdown(DNServer *server)
{
	AuthServerHelper* serverProxy = GetAuthServer();

	if (DNWebProxyHelper* serverSock = serverProxy->GetSSock())
	{
		if(serverSock->service != nullptr)
		{
			delete serverSock->service;
			serverSock->service = nullptr;
		}
	}

	if (DNClientProxyHelper* clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;
		clientSock->SetRegistEvent(nullptr);
	}

	return true;
}