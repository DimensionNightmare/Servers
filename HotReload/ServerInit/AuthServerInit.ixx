module;
#include <string>

#include "StdMacro.h"
export module AuthServerInit;

import AuthServerHelper;
import MessagePack;
import AuthMessage;
import DNTask;
import Logger;
import Macro;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

export int HandleAuthServerInit(DNServer* server);
export int HandleAuthServerShutdown(DNServer* server);

int HandleAuthServerInit(DNServer* server)
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
					DNPrint(TipCode::TipCode_CliConnOn, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
					clientSock->SetRegistEvent(&AuthMessage::Evt_ReqRegistSrv);
					TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, InitConnectedChannel, clientSock, channel);
				}
				else
				{
					DNPrint(TipCode::TipCode_CliConnOff, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
					if (clientSock->RegistState() == RegistState::Registed)
					{
						clientSock->RegistState() = RegistState::None;
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

				if (packet.dealType == MsgDeal::Res)
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

int HandleAuthServerShutdown(DNServer* server)
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