module;
#include "google/protobuf/message.h"
#include "hv/Channel.h"

export module ControlServerInit;

import DNServer;
import ControlServer;
import ControlServerHelper;
import MessagePack;
import ControlMessage;
import AfxCommon;
import ServerEntityHelper;

#define DNPrint(code, level, ...) LoggerPrint(level, code, __FUNCTION__, ##__VA_ARGS__);

using namespace hv;
using namespace std;
using namespace google::protobuf;

export void HandleControlServerInit(DNServer *server);
export void HandleControlServerShutdown(DNServer *server);

module:private;

void HandleControlServerInit(DNServer *server)
{
	SetControlServer(static_cast<ControlServer*>(server));

	ControlMessageHandle::RegMsgHandle();

	ControlServerHelper* serverProxy = GetControlServer();
	
	if (auto serverSock = serverProxy->GetSSock())
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

				// not used
				if(ServerEntityHelper* entity = channel->getContext<ServerEntityHelper>())
				{
					auto entityMan = serverProxy->GetEntityManager();
					entityMan->RemoveEntity(entity->GetChild()->GetID());
				}
				
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
				DNPrint(12, LoggerLevel::Error, nullptr);
			}
		};

		serverSock->onConnection = onConnection;
		serverSock->onMessage = onMessage;
	}
}

void HandleControlServerShutdown(DNServer *server)
{
	ControlServerHelper* serverProxy = GetControlServer();
	if (auto serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;
	}
}