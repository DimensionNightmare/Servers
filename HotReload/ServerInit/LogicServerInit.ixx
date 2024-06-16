module;
#include <functional>
#include <format>

#include "StdMacro.h"
export module LogicServerInit;

import LogicServerHelper;
import MessagePack;
import LogicMessage;
import DNTask;
import Logger;
import Macro;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

export int HandleLogicServerInit(DNServer* server);
export int HandleLogicServerShutdown(DNServer* server);

int HandleLogicServerInit(DNServer* server)
{
	SetLogicServer(static_cast<LogicServer*>(server));

	LogicMessageHandle::RegMsgHandle();

	LogicServerHelper* serverProxy = GetLogicServer();

	if (DNServerProxyHelper* serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;

		auto onConnection = [serverProxy](const SocketChannelPtr& channel)
			{
				const string& peeraddr = channel->peeraddr();
				if (channel->isConnected())
				{
					DNPrint(TipCode::TipCode_CliConnOn, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
				}
				else
				{
					DNPrint(TipCode::TipCode_CliConnOff, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
					if (ServerEntity* entity = channel->getContext<ServerEntity>())
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
				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);

				if (packet.dealType == MsgDeal::Req)
				{
					LogicMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == MsgDeal::Ret)
				{
					LogicMessageHandle::MsgRetHandle(channel, packet.msgHashId, msgData);
				}
				else if (packet.dealType == MsgDeal::Redir)
				{
					LogicMessageHandle::MsgRedirectHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == MsgDeal::Res)
				{
					if (DNTask<Message*>* task = serverSock->GetMsg(packet.msgId)) //client sock request
					{
						serverSock->DelMsg(packet.msgId);
						task->Resume();

						if (Message* message = task->GetResult())
						{
							if (!message->ParseFromString(msgData))
							{
								task->SetFlag(DNTaskFlag::PaserError);
							}

						}

						task->CallResume();
					}
					else
					{
						DNPrint(ErrCode::ErrCode_MsgFind, LoggerLevel::Error, nullptr);
					}
				}
				else
				{
					DNPrint(ErrCode::ErrCode_MsgDealType, LoggerLevel::Error, nullptr);
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
		auto onConnection = [clientSock, serverProxy](const SocketChannelPtr& channel)
			{
				const string& peeraddr = channel->peeraddr();

				if (channel->isConnected())
				{
					DNPrint(TipCode::TipCode_SrvConnOn, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
					clientSock->SetRegistEvent(&LogicMessage::Evt_ReqRegistSrv);
					TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, InitConnectedChannel, clientSock, channel);

					serverProxy->GetClientEntityManager()->InitSqlConn(clientSock);
				}
				else
				{
					DNPrint(TipCode::TipCode_SrvConnOff, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());

					string origin = format("{}:{}", serverProxy->GetCtlIp(), serverProxy->GetCtlPort());
					if (clientSock->RegistState() == RegistState::Registed || peeraddr != origin)
					{
						clientSock->RegistState() = RegistState::None;

						if (clientSock->hloop())
						{
							clientSock->Timer()->setTimeout(200, [=](uint64_t timerID)
								{
									DNPrint(0, LoggerLevel::Debug, "orgin not match peeraddr %s reclient ~", origin.c_str());
									TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, RedirectClient, clientSock, serverProxy->GetCtlPort(), serverProxy->GetCtlIp());

								});
						}
					}

					clientSock->RegistType() = 0;
				}

				if (clientSock->isReconnect())
				{
				}
			};

		auto onMessage = [clientSock](const SocketChannelPtr& channel, Buffer* buf)
			{
				MessagePacket packet;
				memcpy(&packet, buf->data(), MessagePacket::PackLenth);
				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);

				if (packet.dealType == MsgDeal::Req)
				{
					LogicMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == MsgDeal::Ret)
				{
					LogicMessageHandle::MsgRetHandle(channel, packet.msgHashId, msgData);
				}
				else if (packet.dealType == MsgDeal::Redir)
				{
					LogicMessageHandle::MsgRedirectHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == MsgDeal::Res)
				{
					if (DNTask<Message*>* task = clientSock->GetMsg(packet.msgId)) //client sock request
					{
						clientSock->DelMsg(packet.msgId);
						task->Resume();

						if (Message* message = task->GetResult())
						{
							if (!message->ParseFromString(msgData))
							{
								task->SetFlag(DNTaskFlag::PaserError);
							}

						}

						task->CallResume();
					}
					else
					{
						DNPrint(ErrCode::ErrCode_MsgFind, LoggerLevel::Error, nullptr);
					}
				}
				else
				{
					DNPrint(ErrCode::ErrCode_MsgDealType, LoggerLevel::Error, nullptr);
				}
			};

		clientSock->onConnection = onConnection;
		clientSock->onMessage = onMessage;
	}

	return serverProxy->InitDatabase();
}

int HandleLogicServerShutdown(DNServer* server)
{
	LogicServerHelper* serverProxy = GetLogicServer();

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

	serverProxy->ClearNosqlProxy();

	if (ClientEntityManagerHelper* entityMan = serverProxy->GetClientEntityManager())
	{
		entityMan->ClearNosqlProxy();
	}

	return true;
}