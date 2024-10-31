module;
#include "StdMacro.h"
export module AuthServerInit;

import AuthServerHelper;
import FuncHelper;
import AuthMessage;
import DNTask;
import Logger;
import Macro;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNServer;
import DNWebProxyHelper;
import MessagePack;

export int HandleAuthServerInit(DNServer* server)
{
	SetAuthServer(static_cast<AuthServer*>(server));

	AuthServerHelper* serverProxy = GetAuthServer();

	if (DNWebProxyHelper* serverSock = serverProxy->GetSSock())
	{
		HttpService* service = new HttpService();

		AuthMessageHandle::RegApiHandle(service);

		serverSock->registerHttpService(service);
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
					DNPrint(TipCode::TipCode_CliConnOn, EMLoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
					clientSock->SetRegistEvent(&AuthMessage::Evt_ReqRegistSrv);
					TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, InitConnectedChannel, clientSock, channel);
				}
				else
				{
					DNPrint(TipCode::TipCode_CliConnOff, EMLoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
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

				if (packet.dealType == EMMsgDeal::Res)
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

	return serverProxy->InitDatabase();
}

export int HandleAuthServerShutdown(DNServer* server)
{
	AuthServerHelper* serverProxy = GetAuthServer();

	if (DNClientProxyHelper* clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;
		clientSock->SetRegistEvent(nullptr);

		// web use clientMsg
		clientSock->MsgMapClear();
	}

	if (DNWebProxyHelper* serverSock = serverProxy->GetSSock())
	{
		if (serverSock->service != nullptr)
		{
			HttpService* temp = serverSock->service;
			serverSock->service = nullptr;
			delete temp;
		}
	}

	return true;
}
