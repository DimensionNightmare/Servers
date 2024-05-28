module;
#include <functional>
#include "google/protobuf/message.h"
#include "hv/Channel.h"

#include "StdMacro.h"
#include "Common/Common.pb.h"
export module GlobalServerInit;

import GlobalServerHelper;
import MessagePack;
import GlobalMessage;
import DNTask;
import Logger;
import Macro;

using namespace hv;
using namespace std;
using namespace google::protobuf;

export int HandleGlobalServerInit(DNServer* server);
export int HandleGlobalServerShutdown(DNServer* server);

int HandleGlobalServerInit(DNServer* server)
{
	SetGlobalServer(static_cast<GlobalServer*>(server));

	GlobalMessageHandle::RegMsgHandle();

	GlobalServerHelper* serverProxy = GetGlobalServer();

	if (DNServerProxyHelper* serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;

		auto onConnection = [serverProxy, serverSock](const SocketChannelPtr& channel)
			{
				const string& peeraddr = channel->peeraddr();
				if (channel->isConnected())
				{
					DNPrint(TipCode_CliConnOn, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
					TICK_MAINSPACE_SIGN_FUNCTION(DNServerProxy, InitConnectedChannel, serverSock, channel);
				}
				else
				{
					DNPrint(TipCode_CliConnOff, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());

					if (ServerEntityHelper* entity = channel->getContext<ServerEntityHelper>())
					{
						ServerEntityManagerHelper* entityMan = serverProxy->GetServerEntityManager();
						entityMan->RemoveEntity(entity->ID());
						channel->setContext(nullptr);
					}
				}
			};

		auto onMessage = [serverSock](const SocketChannelPtr& channel, Buffer* buf)
			{
				MessagePacket packet;
				memcpy(&packet, buf->data(), MessagePacket::PackLenth);
				if (packet.dealType == MsgDeal::Req)
				{
					string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
					GlobalMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == MsgDeal::Ret)
				{
					string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
					GlobalMessageHandle::MsgRetHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == MsgDeal::Res)
				{
					if (DNTask<Message*>* task = serverSock->GetMsg(packet.msgId)) //client sock request
					{
						serverSock->DelMsg(packet.msgId);
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

		serverSock->onConnection = onConnection;
		serverSock->onMessage = onMessage;
	}

	if (DNClientProxyHelper* clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;

		auto onConnection = [clientSock](const SocketChannelPtr& channel)
			{
				const string& peeraddr = channel->peeraddr();

				if (channel->isConnected())
				{
					DNPrint(TipCode_SrvConnOn, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
					clientSock->SetRegistEvent(&GlobalMessage::Evt_ReqRegistSrv);
					TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, InitConnectedChannel, clientSock, channel);
				}
				else
				{
					DNPrint(TipCode_SrvConnOff, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
					if (clientSock->RegistState() == RegistState::Registed)
					{
						clientSock->RegistState() = RegistState::None;
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
					GlobalMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
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

	return true;

}

int HandleGlobalServerShutdown(DNServer* server)
{
	GlobalServerHelper* serverProxy = GetGlobalServer();

	if (DNServerProxyHelper* serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;

		serverSock->MsgMapClear();
	}

	if (DNClientProxyHelper* clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;
		clientSock->SetRegistEvent(nullptr);

		clientSock->MsgMapClear();
	}

	return true;
}