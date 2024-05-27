module;
#include "google/protobuf/message.h"
#include "hv/Channel.h"
#include "hv/HttpServer.h"

#include "StdMacro.h"
#include "Common/Common.pb.h"
export module AuthServerInit;

import AuthServerHelper;
import MessagePack;
import AuthMessage;
import DNTask;
import Logger;
import Macro;

using namespace hv;
using namespace std;
using namespace google::protobuf;

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
					DNPrint(TipCode_CliConnOn, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
					channel->setHeartbeat(4000, std::bind(&DNClientProxy::TickHeartbeat, clientSock));
					clientSock->SetRegistEvent(&AuthMessage::Evt_ReqRegistSrv);
					TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, StartRegist, clientSock);

					channel->setWriteTimeout(12000);
				}
				else
				{
					DNPrint(TipCode_CliConnOff, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
					if (clientSock->RegistState() == RegistState::Registed)
					{
						clientSock->RegistState() = RegistState::None;
					}
				}

				if (clientSock->isReconnect())
				{

				}
			};

		auto onMessage = [clientSock](const SocketChannelPtr& channel, Buffer* buf)
			{
				MessagePacket packet;
				memcpy(&packet, buf->data(), MessagePacket::PackLenth);
				if (packet.dealType == MsgDeal::Res)
				{
					if (DNTask<Message*>* task = clientSock->GetMsg(packet.msgId)) //client sock request
					{
						clientSock->DelMsg(packet.msgId);
						task->Resume();

						if (Message* message = task->GetResult())
						{
							bool parserError = false;
							//Support Combine
							if (task->HasFlag(DNTaskFlag::Combine))
							{
								Message* merge = message->New();
								if (merge->ParseFromArray(buf->base + MessagePacket::PackLenth, packet.pkgLenth))
								{
									message->MergeFrom(*merge);
								}

								delete merge;
							}
							else
							{
								parserError = !message->ParseFromArray(buf->base + MessagePacket::PackLenth, packet.pkgLenth);
							}

							if (parserError)
							{
								task->SetFlag(DNTaskFlag::PaserError);
							}

						}

						task->CallResume();
					}
					else
					{
						DNPrint(ErrCode_MsgFind, LoggerLevel::Error, nullptr);
					}
				}
				else
				{
					DNPrint(ErrCode_MsgDealType, LoggerLevel::Error, nullptr);
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
			delete serverSock->service;
			serverSock->service = nullptr;
		}
	}

	return true;
}