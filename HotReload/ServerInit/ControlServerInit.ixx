module;
export module ControlServerInit;

import ControlServerHelper;
import FuncHelper;
import ControlMessage;
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

export int HandleControlServerInit(DNServer* server)
{
	SetControlServer(static_cast<ControlServer*>(server));

	ControlMessageHandle::RegMsgHandle();

	ControlServerHelper* serverProxy = GetControlServer();

	if (DNServerProxyHelper* serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;

		auto onConnection = [serverProxy, serverSock](const hv::SocketChannelPtr& channel)
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

					// not used
					if (ServerEntity* entity = channel->getContext<ServerEntity>())
					{
						ServerEntityManagerHelper* entityMan = serverProxy->GetServerEntityManager();
						entityMan->RemoveEntity(entity->ID());
						channel->setContext(nullptr);
					}

				}
			};

		auto onMessage = [serverSock](const hv::SocketChannelPtr& channel, hv::Buffer* buf)
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
					ControlMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Ret)
				{
					ControlMessageHandle::MsgRetHandle(channel, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Redir)
				{
					ControlMessageHandle::MsgRedirectHandle(channel, packet.msgId, packet.msgHashId, msgData);
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

	return true;
}

export int HandleControlServerShutdown(DNServer* server)
{
	ControlServerHelper* serverProxy = GetControlServer();
	if (DNServerProxyHelper* serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;

		serverSock->MsgMapClear();
	}

	return true;
}
