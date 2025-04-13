module;
export module GlobalServerInit;

import GlobalServerHelper;
import FuncHelper;
import GlobalMessage;
import DNTask;
import Logger;
import DllUtils;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNServer;
import DNServerProxyHelper;
import MessagePack;
import std.compat;

#define FUNCPLACE(func) #func, func

export int HandleGlobalServerInit(DNServer* server)
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
				const std::string& peeraddr = channel->peeraddr();
				if (channel->isConnected())
				{
					LoggerPrint()(EL10nCode_CliConnOn, peeraddr, channel->fd(), channel->id());
					TickMainSpaceDll(serverSock, FUNCPLACE(&DNServerProxy::InitConnectedChannel),  channel);
				}
				else
				{
					LoggerPrint()(EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());

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

				LoggerPrint()(ELogLevel_Debug, "s {} Recv type={} With Mid:{}", channel->peeraddr(), static_cast<int>(packet.dealType), packet.msgId);

				if(packet.pkgLenth > 2 * 1024)
				{
					LoggerPrint()(ELogLevel_Debug, "Recv byte len limit={}", packet.pkgLenth);
					return;
				}

				std::string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);

				if (packet.dealType == EMMsgDeal::Req)
				{
					GlobalMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Ret)
				{
					GlobalMessageHandle::MsgRetHandle(channel, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Redir)
				{
					GlobalMessageHandle::MsgRedirectHandle(channel, packet.msgId, packet.msgHashId, msgData);
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
						LoggerPrint()(EL10nCode_MsgFind);
					}
				}
				else
				{
					LoggerPrint()(EL10nCode_MsgDealType);
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
				const std::string& peeraddr = channel->peeraddr();

				if (channel->isConnected())
				{
					LoggerPrint()(EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());
					clientSock->SetRegistEvent(&GlobalMessage::Evt_ReqRegistSrv);
					TickMainSpaceDll(clientSock, FUNCPLACE(&DNClientProxy::InitConnectedChannel),  channel);
				}
				else
				{
					LoggerPrint()(EL10nCode_SrvConnOff, peeraddr, channel->fd(), channel->id());
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

				LoggerPrint()(ELogLevel_Debug, "c {} Recv type={} With Mid:{}", channel->peeraddr(), static_cast<int>(packet.dealType), packet.msgId);

				if(packet.pkgLenth > 2 * 1024)
				{
					LoggerPrint()(ELogLevel_Debug, "Recv byte len limit={}", packet.pkgLenth);
					return;
				}
				
				std::string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);

				if (packet.dealType == EMMsgDeal::Req)
				{
					GlobalMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Redir)
				{
					GlobalMessageHandle::MsgRedirectHandle(channel, packet.msgId, packet.msgHashId, msgData);
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
						LoggerPrint()(EL10nCode_MsgFind);
					}
				}
				else
				{
					LoggerPrint()(EL10nCode_MsgDealType);
				}
			};

		clientSock->onConnection = onConnection;
		clientSock->onMessage = onMessage;
	}

	return true;

}

export int HandleGlobalServerShutdown(DNServer* server)
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
