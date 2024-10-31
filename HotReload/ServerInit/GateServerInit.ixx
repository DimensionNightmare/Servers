module;
#include "StdMacro.h"
export module GateServerInit;

import GateServerHelper;
import FuncHelper;
import GateMessage;
import NetEntity;
import DNTask;
import Logger;
import Macro;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNServer;
import MessagePack;
import DNServerProxyHelper;

export int HandleGateServerInit(DNServer* server)
{
	SetGateServer(static_cast<GateServer*>(server));

	GateMessageHandle::RegMsgHandle();

	GateServerHelper* serverProxy = GetGateServer();

	if (DNServerProxyHelper* serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;

		auto onConnection = [serverSock, serverProxy](const SocketChannelPtr& channel)
			{
				const string& peeraddr = channel->peeraddr();
				if (channel->isConnected())
				{
					DNPrint(TipCode::TipCode_CliConnOn, EMLoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
					TICK_MAINSPACE_SIGN_FUNCTION(DNServerProxy, InitConnectedChannel, serverSock, channel);
				}
				else
				{
					DNPrint(TipCode::TipCode_CliConnOff, EMLoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
					if (Entity* entity = channel->getContext<Entity>())
					{
						switch (entity->GetEntityType())
						{
							case EMEntityType::Server:
								serverProxy->ServerEntityCloseEvent(entity);
								break;
							case EMEntityType::Proxy:
								serverProxy->ProxyEntityCloseEvent(entity);
								break;
							default:
								break;

						}

					}
				}
			};

		auto onMessage = [serverSock](const SocketChannelPtr& channel, Buffer* buf)
			{
				MessagePacket packet;
				memcpy(&packet, buf->data(), MessagePacket::PackLenth);

				DNPrint(0, EMLoggerLevel::Debug, "s %s Recv type=%d With Mid:%u", channel->peeraddr().c_str(), packet.dealType, packet.msgId);

				if(packet.pkgLenth > 2 * 1024)
				{
					DNPrint(0, EMLoggerLevel::Debug, "Recv byte len limit=%u", packet.pkgLenth);
					return;
				}

				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);

				if (packet.dealType == EMMsgDeal::Req)
				{
					GateMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Ret)
				{
					GateMessageHandle::MsgRetHandle(channel, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Redir)
				{
					GateMessageHandle::MsgRedirectHandle(channel, packet.msgId, packet.msgHashId, msgData);
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
						DNPrint(ErrCode::ErrCode_MsgFind, EMLoggerLevel::Error, nullptr);
					}
				}
				else
				{
					DNPrint(ErrCode::ErrCode_MsgDealType, EMLoggerLevel::Error, nullptr);
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
					DNPrint(TipCode::TipCode_SrvConnOn, EMLoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
					clientSock->SetRegistEvent(&GateMessage::Evt_ReqRegistSrv);
					TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, InitConnectedChannel, clientSock, channel);
				}
				else
				{
					DNPrint(TipCode::TipCode_SrvConnOff, EMLoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
					if (clientSock->EMRegistState() == EMRegistState::Registed)
					{
						clientSock->EMRegistState() = EMRegistState::None;
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

				DNPrint(0, EMLoggerLevel::Debug, "c %s Recv type=%d With Mid:%u", channel->peeraddr().c_str(), packet.dealType, packet.msgId);

				if(packet.pkgLenth > 2 * 1024)
				{
					DNPrint(0, EMLoggerLevel::Debug, "Recv byte len limit=%u", packet.pkgLenth);
					return;
				}
				
				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);

				if (packet.dealType == EMMsgDeal::Req)
				{
					GateMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Redir)
				{
					GateMessageHandle::MsgRedirectHandle(channel, packet.msgId, packet.msgHashId, msgData);
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
						DNPrint(ErrCode::ErrCode_MsgFind, EMLoggerLevel::Error, nullptr);
					}
				}
				else
				{
					DNPrint(ErrCode::ErrCode_MsgDealType, EMLoggerLevel::Error, nullptr);
				}
			};

		clientSock->onConnection = onConnection;
		clientSock->onMessage = onMessage;
	}

	return true;
}

export int HandleGateServerShutdown(DNServer* server)
{
	GateServerHelper* serverProxy = GetGateServer();

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
