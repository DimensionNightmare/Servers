module;
#include "StdMacro.h"
export module LogicServerInit;

import LogicServerHelper;
import FuncHelper;
import LogicMessage;
import DNTask;
import Logger;
import DllUtils;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNServer;
import MessagePack;
import DNServerProxyHelper;

export int HandleLogicServerInit(DNServer* server)
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
				const std::string& peeraddr = channel->peeraddr();
				if (channel->isConnected())
				{
					DNPrintCode(EL10nCode_CliConnOn, peeraddr.c_str(), channel->fd(), channel->id());
				}
				else
				{
					DNPrintCode(EL10nCode_CliConnOff, peeraddr.c_str(), channel->fd(), channel->id());
					if (RoomEntity* entity = channel->getContext<RoomEntity>())
					{
						RoomEntityManagerHelper* entityMan = serverProxy->GetRoomEntityManager();
						entityMan->RemoveEntity(entity->ID());
						channel->setContext(nullptr);
					}
				}
			};

		auto onMessage = [serverSock](const SocketChannelPtr& channel, Buffer* buf)
			{
				MessagePacket packet;
				memcpy(&packet, buf->data(), MessagePacket::PackLenth);

				DNPrint(ELogLevel_Debug, "s %s Recv type=%d With Mid:%u", channel->peeraddr().c_str(), packet.dealType, packet.msgId);

				if(packet.pkgLenth > 2 * 1024)
				{
					DNPrint(ELogLevel_Debug, "Recv byte len limit=%u", packet.pkgLenth);
					return;
				}
				
				std::string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);

				if (packet.dealType == EMMsgDeal::Req)
				{
					LogicMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Ret)
				{
					LogicMessageHandle::MsgRetHandle(channel, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Redir)
				{
					LogicMessageHandle::MsgRedirectHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Res)
				{
					if (DNTask<Message*>* task = serverSock->GetMsg(packet.msgId)) //client sock request
					{
						serverSock->DelMsg(packet.msgId);
						task->Resume();

						if (Message* message = task->GetResult())
						{
							if (!message->ParseFromString(msgData))
							{
								task->SetFlag(EMDNTaskFlag::PaserError);
							}

						}

						task->CallResume();
					}
					else
					{
						DNPrintCode(EL10nCode_MsgFind);
					}
				}
				else
				{
					DNPrintCode(EL10nCode_MsgDealType);
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
				const std::string& peeraddr = channel->peeraddr();

				if (channel->isConnected())
				{
					DNPrintCode(EL10nCode_SrvConnOn, peeraddr.c_str(), channel->fd(), channel->id());
					clientSock->SetRegistEvent(&LogicMessage::Evt_ReqRegistSrv);
					TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, InitConnectedChannel, clientSock, channel);

					serverProxy->GetClientEntityManager()->InitSqlConn(clientSock);
				}
				else
				{
					DNPrintCode(EL10nCode_SrvConnOff, peeraddr.c_str(), channel->fd(), channel->id());

					std::string origin = std::format("{}:{}", serverProxy->GetCtlIp(), serverProxy->GetCtlPort());
					if (clientSock->EMRegistState() == EMRegistState::Registed || peeraddr != origin)
					{
						clientSock->EMRegistState() = EMRegistState::None;

						if (clientSock->isConnected())
						{
							clientSock->Timer()->setTimeout(200, [=](uint64_t timerID)
								{
									DNPrint(ELogLevel_Debug, "orgin not match peeraddr %s reclient ~", origin.c_str());
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

				DNPrint(ELogLevel_Debug, "c %s Recv type=%d With Mid:%u", channel->peeraddr().c_str(), packet.dealType, packet.msgId);

				if(packet.pkgLenth > 2 * 1024)
				{
					DNPrint(ELogLevel_Debug, "Recv byte len limit=%u", packet.pkgLenth);
					return;
				}

				std::string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);

				if (packet.dealType == EMMsgDeal::Req)
				{
					LogicMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Ret)
				{
					LogicMessageHandle::MsgRetHandle(channel, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Redir)
				{
					LogicMessageHandle::MsgRedirectHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Res)
				{
					if (DNTask<Message*>* task = clientSock->GetMsg(packet.msgId)) //client sock request
					{
						clientSock->DelMsg(packet.msgId);
						task->Resume();

						if (Message* message = task->GetResult())
						{
							if (!message->ParseFromString(msgData))
							{
								task->SetFlag(EMDNTaskFlag::PaserError);
							}

						}

						task->CallResume();
					}
					else
					{
						DNPrintCode(EL10nCode_MsgFind);
					}
				}
				else
				{
					DNPrintCode(EL10nCode_MsgDealType);
				}
			};

		clientSock->onConnection = onConnection;
		clientSock->onMessage = onMessage;
	}

	return serverProxy->InitDatabase();
}

export int HandleLogicServerShutdown(DNServer* server)
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
