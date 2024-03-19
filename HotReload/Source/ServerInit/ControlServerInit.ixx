module;
#include "StdAfx.h"
#include "google/protobuf/message.h"
#include "hv/Channel.h"

export module ControlServerInit;

import DNServer;
import ControlServer;
import ControlServerHelper;
import MessagePack;
import ControlMessage;
import ServerEntityHelper;


using namespace hv;
using namespace std;
using namespace google::protobuf;

export int HandleControlServerInit(DNServer *server);
export int HandleControlServerShutdown(DNServer *server);


int HandleControlServerInit(DNServer *server)
{
	SetControlServer(static_cast<ControlServer*>(server));

	ControlMessageHandle::RegMsgHandle();

	ControlServerHelper* serverProxy = GetControlServer();
	
	if (auto serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;

		auto onConnection = [serverProxy,serverSock](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();
			if (channel->isConnected())
			{
				DNPrint(2, LoggerLevel::Debug, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
				// if not regist
				size_t timerId = serverSock->Timer()->setTimeout(5000, std::bind(&DNServerProxy::ChannelTimeoutTimer, serverSock, placeholders::_1));
				serverSock->AddTimerRecord(timerId, channel->id());
				// if not recive data
				channel->setReadTimeout(15000);
			}
			else
			{
				DNPrint(3, LoggerLevel::Debug, nullptr, peeraddr.c_str(), channel->fd(), channel->id());

				// not used
				if(ServerEntityHelper* entity = channel->getContext<ServerEntityHelper>())
				{
					auto entityMan = serverProxy->GetEntityManager();
					entityMan->RemoveEntity(entity->GetChild()->ID());
				}
				
			}
		};

		auto onMessage = [serverProxy](const SocketChannelPtr &channel, Buffer *buf) 
		{
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.dealType == MsgDeal::Req)
			{
				string msgData((char*)buf->data() + MessagePacket::PackLenth, packet.pkgLenth);
				ControlMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else if(packet.dealType == MsgDeal::Ret)
			{
				string msgData((char*)buf->data() + MessagePacket::PackLenth, packet.pkgLenth);
				ControlMessageHandle::MsgRetHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else if(packet.dealType == MsgDeal::Res)
			{
				auto servSock = serverProxy->GetSSock();

				if(DNTask<Message *>* task = servSock->GetMsg(packet.msgId)) //client sock request
				{
					servSock->DelMsg(packet.msgId);
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

		serverSock->onConnection = onConnection;
		serverSock->onMessage = onMessage;
	}

	return true;
}

int HandleControlServerShutdown(DNServer *server)
{
	ControlServerHelper* serverProxy = GetControlServer();
	if (auto serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;
	}

	return true;
}