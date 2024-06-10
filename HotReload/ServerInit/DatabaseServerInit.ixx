module;
#include <functional>
#include <format>
#include "google/protobuf/message.h"
#include "hv/Channel.h"

#include "StdMacro.h"
#include "Common/Common.pb.h"
export module DatabaseServerInit;

import DatabaseServerHelper;
import MessagePack;
import DatabaseMessage;
import DNTask;
import Logger;
import Macro;

using namespace hv;
using namespace std;
using namespace google::protobuf;

export int HandleDatabaseServerInit(DNServer* server);
export int HandleDatabaseServerShutdown(DNServer* server);

int HandleDatabaseServerInit(DNServer* server)
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
					DNPrint(TipCode_SrvConnOn, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());
					clientSock->SetRegistEvent(&DatabaseMessage::Evt_ReqRegistSrv);
					TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, InitConnectedChannel, clientSock, channel);
				}
				else
				{
					DNPrint(TipCode_SrvConnOff, LoggerLevel::Normal, nullptr, peeraddr.c_str(), channel->fd(), channel->id());

					string origin = format("{}:{}", serverProxy->GetCtlIp(), serverProxy->GetCtlPort());
					if (clientSock->RegistState() == RegistState::Registed || peeraddr != origin)
					{
						clientSock->RegistState() = RegistState::None;

						if (clientSock->hloop())
						{
							clientSock->Timer()->setTimeout(200, [=](uint64_t timerID)
								{
									DNPrint(0, LoggerLevel::Debug, "orgin not match peeraddr %s reclient ~", origin.c_str());
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
				string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);

				if (packet.dealType == MsgDeal::Req)
				{
					DatabaseMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == MsgDeal::Ret)
				{
					DatabaseMessageHandle::MsgRetHandle(channel, packet.msgHashId, msgData);
				}
				else if (packet.dealType == MsgDeal::Res)
				{
					if (DNTask<Message*>* task = clientSock->GetMsg(packet.msgId)) // client sock request
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

int HandleDatabaseServerShutdown(DNServer* server)
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