module;
#include "StdMacro.h"
export module DatabaseServerInit;

import DatabaseServerHelper;
import FuncHelper;
import DatabaseMessage;
import DNTask;
import Logger;
import Macro;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNServer;
import DNClientProxyHelper;
import MessagePack;

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
				const string& peeraddr = channel->peeraddr();

				if (channel->isConnected())
				{
					DNPrint(TipCode::TipCode_SrvConnOn, EMLoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
					clientSock->SetRegistEvent(&DatabaseMessage::Evt_ReqRegistSrv);
					TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, InitConnectedChannel, clientSock, channel);
				}
				else
				{
					DNPrint(TipCode::TipCode_SrvConnOff, EMLoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());

					string origin = format("{}:{}", serverProxy->GetCtlIp(), serverProxy->GetCtlPort());
					if (clientSock->EMRegistState() == EMRegistState::Registed || peeraddr != origin)
					{
						clientSock->EMRegistState() = EMRegistState::None;

						if (clientSock->hloop())
						{
							clientSock->Timer()->setTimeout(200, [=](uint64_t timerID)
								{
									DNPrint(0, EMLoggerLevel::Debug, "orgin not match peeraddr %s reclient ~", origin.c_str());
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

				DNPrint(0, EMLoggerLevel::Debug, "c %s Recv type=%d With Mid:%u", channel->peeraddr().c_str(), packet.dealType, packet.msgId);

				if(packet.pkgLenth > 2 * 1024)
				{
					DNPrint(0, EMLoggerLevel::Debug, "Recv byte len limit=%u", packet.pkgLenth);
					return;
				}
				
				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);

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
