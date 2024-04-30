module;
#include "google/protobuf/message.h"
#include "hv/Channel.h"

#include "StdAfx.h"
export module ControlServerInit;

import ControlServerHelper;
import MessagePack;
import ControlMessage;
import DNTask;

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
	
	if (DNServerProxyHelper* serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;

		auto onConnection = [serverProxy, serverSock](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();
			if (channel->isConnected())
			{
				DNPrint(TipCode_CliConnOn, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
				// if not regist
				size_t timerId = serverSock->Timer()->setTimeout(5000, std::bind(&DNServerProxy::ChannelTimeoutTimer, static_cast<DNServerProxy*>(serverSock), placeholders::_1));
				serverSock->AddTimerRecord(timerId, channel->id());
				// if not recive data
				channel->setReadTimeout(15000);
			}
			else
			{
				DNPrint(TipCode_CliConnOff, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());

				// not used
				if(ServerEntityHelper* entity = channel->getContext<ServerEntityHelper>())
				{
					ServerEntityManagerHelper*  entityMan = serverProxy->GetServerEntityManager();
					entityMan->RemoveEntity(entity->ID());
					channel->setContext(nullptr);
				}
				
			}
		};

		auto onMessage = [serverSock](const SocketChannelPtr &channel, Buffer *buf) 
		{
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.dealType == MsgDeal::Req)
			{
				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
				ControlMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else if(packet.dealType == MsgDeal::Ret)
			{
				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
				ControlMessageHandle::MsgRetHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else if(packet.dealType == MsgDeal::Res)
			{
				if(DNTask<Message>* task = serverSock->GetMsg(packet.msgId)) //client sock request
				{
					serverSock->DelMsg(packet.msgId);
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

		serverSock->onConnection = onConnection;
		serverSock->onMessage = onMessage;
	}

	return true;
}

int HandleControlServerShutdown(DNServer *server)
{
	ControlServerHelper* serverProxy = GetControlServer();
	if (DNServerProxyHelper* serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;
	}

	return true;
}