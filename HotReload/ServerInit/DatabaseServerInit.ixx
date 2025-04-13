module;
export module DatabaseServerInit;

import DatabaseServerHelper;
import FuncHelper;
import DatabaseMessage;
import DNTask;
import Logger;
import DllUtils;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNServer;
import DNClientProxyHelper;
import MessagePack;
import std.compat;

#define FUNCPLACE(func) #func, func

export int HandleDatabaseServerInit(DNServer* server)
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
				const std::string& peeraddr = channel->peeraddr();

				if (channel->isConnected())
				{
					LoggerPrint()(EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());
					clientSock->SetRegistEvent(&DatabaseMessage::Evt_ReqRegistSrv);
					TickMainSpaceDll(clientSock, FUNCPLACE(&DNClientProxy::InitConnectedChannel),  channel);
				}
				else
				{
					LoggerPrint()(EL10nCode_SrvConnOff, peeraddr, channel->fd(), channel->id());

					std::string origin = std::format("{}:{}", serverProxy->GetCtlIp(), serverProxy->GetCtlPort());
					if (clientSock->EMRegistState() == EMRegistState::Registed || peeraddr != origin)
					{
						clientSock->EMRegistState() = EMRegistState::None;

						if (clientSock->isConnected())
						{
							clientSock->Timer()->setTimeout(200, [=](uint64_t timerID)
								{
									LoggerPrint()(ELogLevel_Debug, "orgin not match peeraddr {} reclient ~", origin);
									TickMainSpaceDll(clientSock, FUNCPLACE(&DNClientProxy::RedirectClient),  serverProxy->GetCtlPort(), serverProxy->GetCtlIp());
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

				LoggerPrint()(ELogLevel_Debug, "c {} Recv type={} With Mid:{}", channel->peeraddr(), static_cast<int>(packet.dealType), packet.msgId);

				if(packet.pkgLenth > 2 * 1024)
				{
					LoggerPrint()(ELogLevel_Debug, "Recv byte len limit={}", packet.pkgLenth);
					return;
				}
				
				std::string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);

				if (packet.dealType == EMMsgDeal::Req)
				{
					DatabaseMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Ret)
				{
					DatabaseMessageHandle::MsgRetHandle(channel, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Res)
				{
					if (DNTask<Message*>* task = clientSock->GetMsg(packet.msgId)) // client sock request
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

	return serverProxy->InitDatabase();
}

export int HandleDatabaseServerShutdown(DNServer* server)
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
