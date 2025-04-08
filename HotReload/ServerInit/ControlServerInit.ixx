module;
#include "StdMacro.h"
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

export int HandleControlServerInit(DNServer* server)
{
	SetControlServer(static_cast<ControlServer*>(server));

	ControlMessageHandle::RegMsgHandle();

	ControlServerHelper* serverProxy = GetControlServer();

	if (DNServerProxyHelper* serverSock = serverProxy->GetSSock())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;

		auto onConnection = [serverProxy, serverSock](const SocketChannelPtr& channel)
			{
				const std::string& peeraddr = channel->peeraddr();
				if (channel->isConnected())
				{
					DNPrintCode(EL10nCode_CliConnOn, peeraddr.c_str(), channel->fd(), channel->id());
					TICK_MAINSPACE_SIGN_FUNCTION(DNServerProxy, InitConnectedChannel, serverSock, channel);
				}
				else
				{
					DNPrintCode(EL10nCode_CliConnOff, peeraddr.c_str(), channel->fd(), channel->id());

					// not used
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

				DNPrint(ELogLevel_Debug, "s %s Recv type=%d With Mid:%u", channel->peeraddr().c_str(), packet.dealType, packet.msgId);

				if(packet.pkgLenth > 2 * 1024)
				{
					DNPrint(ELogLevel_Debug, "Recv byte len limit=%u", packet.pkgLenth);
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
