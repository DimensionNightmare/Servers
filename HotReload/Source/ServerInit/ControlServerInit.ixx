module;
#include "google/protobuf/dynamic_message.h"

#include "hv/Channel.h"
#include "hv/hloop.h"
export module ControlServerInit;

import ControlMessage;
import MessagePack;
import ControlServer;
import ControlServerHelper;

using namespace hv;
using namespace std;
using namespace google::protobuf;

export void HandleControlServerInit(ControlServer *server);
export void HandleControlServerShutdown(ControlServer *server);

module:private;

void HandleControlServerInit(ControlServer *server)
{
	SetControlServer(server);
	
	if (auto sSock = server->GetSSock())
	{
		auto onConnection = [&](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();
			if (channel->isConnected())
			{
				printf("%s-> %s connected! connfd=%d id=%d \n", __FUNCTION__, peeraddr.c_str(), channel->fd(), channel->id());
			}
			else
			{
				printf("%s-> %s disconnected! connfd=%d id=%d \n", __FUNCTION__, peeraddr.c_str(), channel->fd(), channel->id());
			}
		};

		auto onMessage = [](const SocketChannelPtr &channel, Buffer *buf) {
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.dealType == MsgDeal::Req)
			{
				string msgData((char*)buf->data() + MessagePacket::PackLenth, packet.pkgLenth);
				ControlMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
		};

		sSock->onConnection = onConnection;
		sSock->onMessage = onMessage;

		ControlMessageHandle::RegMsgHandle();
	}
}

void HandleControlServerShutdown(ControlServer *server)
{
	if (auto sSock = server->GetSSock())
	{
		sSock->onConnection = nullptr;
		sSock->onMessage = nullptr;
	}
}