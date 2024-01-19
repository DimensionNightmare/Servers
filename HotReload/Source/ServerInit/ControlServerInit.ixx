module;
#include "google/protobuf/Message.h"
#include "hv/Channel.h"

export module ControlServerInit;

import DNServer;
import ControlServer;
import ControlServerHelper;
import MessagePack;
import ControlMessage;
import AfxCommon;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);

using namespace hv;
using namespace std;
using namespace google::protobuf;

export void HandleControlServerInit(DNServer *server);
export void HandleControlServerShutdown(DNServer *server);

module:private;

void HandleControlServerInit(DNServer *server)
{
	SetControlServer(static_cast<ControlServer*>(server));

	auto serverProxy = GetControlServer();
	
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

				// 
				auto entityMan = serverProxy->GetEntityManager();
				entityMan->RemoveEntity<ServerEntityHelper>(channel);
			}
		};

		auto onMessage = [](const SocketChannelPtr &channel, Buffer *buf) 
		{
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.dealType == MsgDeal::Req)
			{
				string msgData((char*)buf->data() + MessagePacket::PackLenth, packet.pkgLenth);
				ControlMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else
			{
				DNPrintErr("packet.dealType not matching! \n");
			}
		};

		sSock->onConnection = onConnection;
		sSock->onMessage = onMessage;

		ControlMessageHandle::RegMsgHandle();
	}
}

void HandleControlServerShutdown(DNServer *server)
{
	auto serverProxy = GetControlServer();
	if (auto sSock = serverProxy->GetSSock())
	{
		sSock->onConnection = nullptr;
		sSock->onMessage = nullptr;
	}
}