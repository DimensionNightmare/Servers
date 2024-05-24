module;
#include <functional>
#include <format>
#include "google/protobuf/message.h"
#include "hv/Channel.h"

#include "StdAfx.h"
export module LogicServerInit;

import LogicServerHelper;
import MessagePack;
import LogicMessage;
import DNTask;

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

	// ServerEntityManagerHelper* entityMan = serverProxy->GetServerEntityManager();
	// DNPrint(0, LoggerLevel::Debug, "ServerEntityManager:%p, ServerEntityManagerHelper:%p", static_cast<ServerEntityManager*>(entityMan), entityMan);
	// DNPrint(0, LoggerLevel::Debug, "ServerEntityManager Func:%p", &LogicServerHelper::GetServerEntityManager);

	if (DNServerProxyHelper* serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;

		auto onConnection = [serverProxy](const SocketChannelPtr &channel)
		{
			const string& peeraddr = channel->peeraddr();
			if (channel->isConnected())
			{
				DNPrint(TipCode_CliConnOn, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
			}
			else
			{
				DNPrint(TipCode_CliConnOff, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
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
				LogicMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else if(packet.dealType == MsgDeal::Ret)
			{
				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
				LogicMessageHandle::MsgRetHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else if(packet.dealType == MsgDeal::Res)
			{
				if(DNTask<Message>* task = serverSock->GetMsg(packet.msgId)) //client sock request
				{
					serverSock->DelMsg(packet.msgId);
					task->Resume();

					if(Message* message = task->GetResult())
					{
						bool parserError = false;
						//Support Combine
						if(task->HasFlag(DNTaskFlag::Combine))
						{
							Message* merge = message->New();
							if(merge->ParseFromArray(buf->base + MessagePacket::PackLenth, packet.pkgLenth))
							{
								message->MergeFrom(*merge);
							}
						
							delete merge;
						}
						else
						{
							parserError = !message->ParseFromArray(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
						}

						if(parserError)
						{
							task->SetFlag(DNTaskFlag::PaserError);
						}
						
					}
					
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


	if (DNClientProxyHelper* clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;
		
		//client will re_create please check
		auto onConnection = [clientSock,serverProxy](const SocketChannelPtr &channel)
		{
			const string& peeraddr = channel->peeraddr();

			if (channel->isConnected())
			{
				DNPrint(TipCode_SrvConnOn, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
				clientSock->SetRegistEvent(&LogicMessage::Evt_ReqRegistSrv);
				clientSock->DNClientProxy::StartRegist();
				channel->setHeartbeat(4000, std::bind(&DNClientProxy::TickHeartbeat, clientSock));

				channel->setWriteTimeout(12000);
			}
			else
			{
				DNPrint(TipCode_SrvConnOff, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
				if(clientSock->RegistState() == RegistState::Registed)
				{
					clientSock->RegistState() = RegistState::None;
				}

				// not orgin
				string origin = format("{}:{}", serverProxy->GetCtlIp(), serverProxy->GetCtlPort());
				if (peeraddr != origin)
				{
					serverProxy->ReClientEvent(serverProxy->GetCtlIp(), serverProxy->GetCtlPort());
				}
			}

			if(clientSock->isReconnect())
			{
				
			}
		};

		auto onMessage = [clientSock](const SocketChannelPtr &channel, Buffer *buf) 
		{
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.dealType == MsgDeal::Req)
			{
				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
				LogicMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else if(packet.dealType == MsgDeal::Ret)
			{
				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
				LogicMessageHandle::MsgRetHandle(channel, packet.msgId, packet.msgHashId, msgData);
			}
			else if(packet.dealType == MsgDeal::Res)
			{
				if(DNTask<Message>* task = clientSock->GetMsg(packet.msgId)) //client sock request
				{
					clientSock->DelMsg(packet.msgId);
					task->Resume();

					if(Message* message = task->GetResult())
					{
						bool parserError = false;
						//Support Combine
						if(task->HasFlag(DNTaskFlag::Combine))
						{
							Message* merge = message->New();
							if(merge->ParseFromArray(buf->base + MessagePacket::PackLenth, packet.pkgLenth))
							{
								message->MergeFrom(*merge);
							}
						
							delete merge;
						}
						else
						{
							parserError = !message->ParseFromArray(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
						}

						if(parserError)
						{
							task->SetFlag(DNTaskFlag::PaserError);
						}
						
					}
					
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

		clientSock->onConnection = onConnection;
		clientSock->onMessage = onMessage;
	}

	return serverProxy->InitDatabase();
}

int HandleLogicServerShutdown(DNServer *server)
{
	LogicServerHelper* serverProxy = GetLogicServer();

	if (DNServerProxy* serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;
	}

	if (DNClientProxyHelper* clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;
		clientSock->SetRegistEvent(nullptr);
	}

	return true;
}