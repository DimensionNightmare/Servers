module;
#include "google/protobuf/Message.h"
#include "hv/Channel.h"

export module AuthServerInit;

import DNServer;
import AuthServer;
import AuthServerHelper;
import MessagePack;
import AuthMessage;
import AfxCommon;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr(), __FUNCTION__, ##__VA_ARGS__);

using namespace hv;
using namespace std;
using namespace google::protobuf;

export void HandleAuthServerInit(DNServer *server);
export void HandleAuthServerShutdown(DNServer *server);

module:private;

void HandleAuthServerInit(DNServer *server)
{
	SetAuthServer(static_cast<AuthServer*>(server));

	auto serverProxy = GetAuthServer();

	if(auto sSock = serverProxy->GetSSock())
	{
		HttpService* service = new HttpService;
		
		AuthMessageHandle::RegApiHandle(service);

		sSock->registerHttpService(service);
	}

	if (auto cSock = serverProxy->GetCSock())
	{
		auto onConnection = [](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();
			auto cProxy = GetAuthServer()->GetCSock();

			cProxy->UpdateClientState(channel->status);

			if (channel->isConnected())
			{
				DNPrint("%s connected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
			}
			else
			{
				DNPrint("%s disconnected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
			}

			if(cProxy->isReconnect())
			{
				
			}
		};

		auto onMessage = [](const SocketChannelPtr &channel, Buffer *buf) 
		{
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.dealType == MsgDeal::Res)
			{
				auto& reqMap = GetAuthServer()->GetCSock()->GetMsgMap();
				if(reqMap.contains(packet.msgId)) //client sock request
				{
					auto task = reqMap.at(packet.msgId);
					reqMap.erase(packet.msgId);
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

void HandleAuthServerShutdown(DNServer *server)
{
	auto serverProxy = GetAuthServer();

	if (auto sSock = serverProxy->GetSSock())
	{
		if(sSock->service != nullptr)
		{
			delete sSock->service;
			sSock->service = nullptr;
		}
	}

	if (auto cSock = serverProxy->GetCSock())
	{
		cSock->onConnection = nullptr;
		cSock->onMessage = nullptr;
		cSock->SetRegistEvent(nullptr);
	}
}