module;
#include "google/protobuf/message.h"
#include "hv/Channel.h"

export module AuthServerInit;

import DNServer;
import AuthServer;
import AuthServerHelper;
import MessagePack;
import AuthMessage;
import AfxCommon;

#define DNPrint(code, level, ...) LoggerPrint(level, code, __FUNCTION__, ##__VA_ARGS__);

using namespace hv;
using namespace std;
using namespace google::protobuf;

export void HandleAuthServerInit(DNServer *server);
export void HandleAuthServerShutdown(DNServer *server);

module:private;

void HandleAuthServerInit(DNServer *server)
{
	SetAuthServer(static_cast<AuthServer*>(server));

	AuthServerHelper* serverProxy = GetAuthServer();

	if(auto serverSock = serverProxy->GetSSock())
	{
		HttpService* service = new HttpService;
		
		AuthMessageHandle::RegApiHandle(service);

		serverSock->registerHttpService(service);
	}

	if (auto clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;

		auto onConnection = [serverProxy](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();

			auto clientSock = serverProxy->GetCSock();

			clientSock->UpdateClientState(channel->status);

			if (channel->isConnected())
			{
				DNPrint(2, LoggerLevel::Debug, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
			}
			else
			{
				DNPrint(3, LoggerLevel::Debug, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
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
			else
			{
				DNPrint(12, LoggerLevel::Error, nullptr);
			}
		};

		clientSock->onConnection = onConnection;
		clientSock->onMessage = onMessage;
		clientSock->SetRegistEvent(&Msg_RegistSrv);
	}

}

void HandleAuthServerShutdown(DNServer *server)
{
	AuthServerHelper* serverProxy = GetAuthServer();

	if (auto serverSock = serverProxy->GetSSock())
	{
		if(serverSock->service != nullptr)
		{
			delete serverSock->service;
			serverSock->service = nullptr;
		}
	}

	if (auto clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;
		clientSock->SetRegistEvent(nullptr);
	}
}