module;
#include <functional>
#include <format>
#include "google/protobuf/message.h"
#include "hv/Channel.h"

#include "StdAfx.h"
export module DatabaseServerInit;

import DatabaseServerHelper;
import MessagePack;
import DatabaseMessage;
import DNTask;

using namespace hv;
using namespace std;
using namespace google::protobuf;

export int HandleDatabaseServerInit(DNServer* server);
export int HandleDatabaseServerShutdown(DNServer* server);

int HandleDatabaseServerInit(DNServer* server)
{
	SetDatabaseServer(static_cast<DatabaseServer*>(server));

	DatabaseMessageHandle::RegMsgHandle();

	DatabaseServerHelper* serverProxy = GetDatabaseServer();

	if (DNClientProxyHelper* clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;

		auto onConnection = [clientSock, serverProxy](const SocketChannelPtr& channel)
			{
				const string& peeraddr = channel->peeraddr();

				if (channel->isConnected())
				{
					DNPrint(TipCode_SrvConnOn, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
					channel->setHeartbeat(4000, std::bind(&DNClientProxy::TickHeartbeat, clientSock));
					clientSock->SetRegistEvent(&DatabaseMessage::Evt_ReqRegistSrv);
					clientSock->DNClientProxy::StartRegist();

					channel->setWriteTimeout(12000);
				}
				else
				{
					DNPrint(TipCode_SrvConnOff, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());

					if (clientSock->RegistState() == RegistState::Registed)
					{
						clientSock->RegistState() = RegistState::None;
					}

					string origin = format("{}:{}", serverProxy->GetCtlIp(), serverProxy->GetCtlPort());
					if (peeraddr != origin)
					{
						serverProxy->ReClientEvent(serverProxy->GetCtlIp(), serverProxy->GetCtlPort());
					}
				}

				if (clientSock->isReconnect())
				{

				}
			};

		auto onMessage = [clientSock](const SocketChannelPtr& channel, Buffer* buf)
			{
				MessagePacket packet;
				memcpy(&packet, buf->data(), MessagePacket::PackLenth);
				if (packet.dealType == MsgDeal::Req)
				{
					string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
					DatabaseMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == MsgDeal::Ret)
				{
					string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
					DatabaseMessageHandle::MsgRetHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == MsgDeal::Res)
				{
					if (DNTask<Message*>* task = clientSock->GetMsg(packet.msgId)) //client sock request
					{
						clientSock->DelMsg(packet.msgId);
						task->Resume();

						if (Message* message = task->GetResult())
						{
							bool parserError = false;
							//Support Combine
							if (task->HasFlag(DNTaskFlag::Combine))
							{
								Message* merge = message->New();
								if (merge->ParseFromArray(buf->base + MessagePacket::PackLenth, packet.pkgLenth))
								{
									message->MergeFrom(*merge);
								}

								delete merge;
							}
							else
							{
								parserError = !message->ParseFromArray(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
							}

							if (parserError)
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

int HandleDatabaseServerShutdown(DNServer* server)
{
	DatabaseServerHelper* serverProxy = GetDatabaseServer();

	if (DNClientProxyHelper* clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;
		clientSock->SetRegistEvent(nullptr);

		clientSock->MsgMapClear();
	}

	return true;
}