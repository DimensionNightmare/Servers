module;
export module AuthServerInit;

import AuthServerHelper;
import FuncHelper;
import AuthMessage;
import DNTask;
import Logger;
import DllUtils;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNServer;
import DNWebProxyHelper;
import MessagePack;
import std.compat;

#define FUNCPLACE(func) #func, func

export int HandleAuthServerInit(DNServer* server)
{
	SetAuthServer(static_cast<AuthServer*>(server));

	AuthServerHelper* serverProxy = GetAuthServer();

	if (DNWebProxyHelper* serverSock = serverProxy->GetSSock())
	{
		hv::HttpService* service = new hv::HttpService();

		AuthMessageHandle::RegApiHandle(service);

		serverSock->registerHttpService(service);
	}

	if (DNClientProxyHelper* clientSock = serverProxy->GetCSock())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;

		auto onConnection = [clientSock](const hv::SocketChannelPtr& channel)
			{
				const std::string& peeraddr = channel->peeraddr();

				if (channel->isConnected())
				{
					LoggerPrint()(EL10nCode_CliConnOn, peeraddr, channel->fd(), channel->id());
					clientSock->SetRegistEvent(&AuthMessage::Evt_ReqRegistSrv);
					TickMainSpaceDll(clientSock, FUNCPLACE(&DNClientProxy::InitConnectedChannel),  channel);
				}
				else
				{
					LoggerPrint()(EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());
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

		auto onMessage = [clientSock](const hv::SocketChannelPtr& channel, hv::Buffer* buf)
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
			hv::HttpService* temp = serverSock->service;
			serverSock->service = nullptr;
			delete temp;
		}
	}

	return true;
}
