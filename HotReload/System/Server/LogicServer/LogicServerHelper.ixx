export module LogicServerHelper;

import ClientProxyHelper;
import ServerProxyHelper;
import RoomEntityManagerHelper;
import ClientEntityManagerHelper;
import MdbProxyHelper;
import Server;
import MessagePack;
import FuncUtils;
import Logger;
import LogicServerMessage;
import StrUtils;

export class LogicServerHelper : public Helper<LogicServerHelper, Server>
{

private:

	LogicServerHelper() = delete;
	~LogicServerHelper() = default;

public:

	ClientProxyHelper::Ptr GetClientProxy()
	{ 
		return GetComponent<ClientProxyHelper>(EMComponentType::ClientProxy);
	}

	ServerProxyHelper::Ptr GetServerProxy() 
	{
		return GetComponent<ServerProxyHelper>(EMComponentType::ServerProxy);
	}

	RoomEntityManagerHelper::Ptr GetRoomEntityManager() 
	{
		return GetComponent<RoomEntityManagerHelper>(EMComponentType::RoomEntityManager);
	}

	ClientEntityManagerHelper::Ptr GetClientEntityManager() 
	{
		return GetComponent<ClientEntityManagerHelper>(EMComponentType::ClientEntityManager);
	}
	
	MdbProxyHelper::Ptr GetMdbProxy() 
	{
		return GetComponent<MdbProxyHelper>(EMComponentType::MdbProxy);
	}

	void HandleServerInit()
	{
		
		if (ServerProxyHelper::CVPtr proxy = GetServerProxy())
		{
			proxy->onConnection = [this](SocketChannel::CVPtr channel)
				{
					ServerProxyHelper::CVPtr proxyHelper = GetServerProxy();

					const std::string& peeraddr = channel->peeraddr();

					World::CVPtr world = GetWorld();

					if (channel->isConnected())
					{
						LoggerPrint::Log(world, EL10nCode_CliConnOn, peeraddr, channel->fd(), channel->id());
					}
					else
					{
						LoggerPrint::Log(world, EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());
						if (RoomEntity::CVPtr entity = channel->GetEntity<RoomEntity>())
						{
							RoomEntityManagerHelper::CVPtr entityMan = GetRoomEntityManager();
							entityMan->DisposeEntity(entity);
						}
					}
				};

			proxy->onMessage = [this](SocketChannel::CVPtr channel, hv::Buffer* buf)
				{
					MessagePacket* packet = MessagePacket::From(buf->data());

					World::CVPtr world = GetWorld();

					if(packet->pkgLenth > 2 * 1024)
					{
						LoggerPrint::Log(world, ELogLevel_Debug, "Recv byte len limit={}", packet->pkgLenth);
						return;
					}
					
					std::string msgData(packet->MsgBegin(), packet->pkgLenth);

					if (packet->dealType == EMMsgDeal::Req)
					{
						ServerMessage::GetMessageHandle()->MsgHandle(world, channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Ret)
					{
						ServerMessage::GetMessageHandle()->MsgRetHandle(world, channel, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Redir)
					{
						ServerMessage::GetMessageHandle()->MsgRedirectHandle(world, channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Res)
					{
						ServerProxyHelper::CVPtr proxyHelper = GetServerProxy();

						if (MsgTask* task = proxyHelper->GetMsg(packet->msgId)) //client sock request
						{
							proxyHelper->DelMsg(packet->msgId);

							if (Message* message = task->GetMessage())
							{
								if (!message->ParseFromString(msgData))
								{
									task->SetFlag(EMTaskFlag::PaserError);
								}

							}

							task->Resume();
						}
						else
						{
							LoggerPrint::Log(world, EL10nCode_MsgFind);
						}
					}
					else
					{
						LoggerPrint::Log(world, EL10nCode_MsgDealType);
					}
				};

		}


		if (ClientProxyHelper::CVPtr proxy = GetClientProxy())
		{
			//client will re_create please check
			proxy->onConnection = [this](SocketChannel::CVPtr channel)
				{
					ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

					const std::string& peeraddr = channel->peeraddr();

					World::CVPtr world = GetWorld();

					if (channel->isConnected())
					{
						LoggerPrint::Log(world, EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());
						
						world->RemoveEvent(EMEventType::ClientProxyRegist);
						world->AddEvent<&LogicServerHelper::HandleClientRegist>(EMEventType::ClientProxyRegist, GetSelf<LogicServerHelper>());
						proxyHelper->InitConnectedChannel(channel);
					}
					else
					{
						LoggerPrint::Log(world, EL10nCode_SrvConnOff, peeraddr, channel->fd(), channel->id());

						std::string originIp;
						if(std::string* param = world->GetParam("ctlIp"))
						{
							originIp = *param;
						}

						std::string originPort;
						if(std::string* param = world->GetParam("ctlPort"))
						{
							originPort = *param;
						}

						std::string origin = std::format("{}:{}", originIp, originPort);
						if (proxyHelper->GetRegistState() == EMRegistState::Registed || peeraddr != origin)
						{
							proxyHelper->SetRegistState(EMRegistState::None);

							if (proxyHelper->isConnected())
							{
								LoggerPrint::Log(world, ELogLevel_Debug, "orgin not match peeraddr {} reclient ~", origin);
								proxyHelper->GetTimer()->SetTimeout(200, [this, originIp, originPort](size_t timerID)
									{
										ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

										proxyHelper->RedirectClient(std::stoi(originPort), originIp);

									});
							}
						}

						proxyHelper->SetRegistType(0);
					}

					if (proxyHelper->isReconnect())
					{
					}
				};

			proxy->onMessage = [this](SocketChannel::CVPtr channel, hv::Buffer* buf)
				{
					MessagePacket* packet = MessagePacket::From(buf->data());

					World::CVPtr world = GetWorld();

					if(packet->pkgLenth > 2 * 1024)
					{
						LoggerPrint::Log(world, ELogLevel_Debug, "Recv byte len limit={}", packet->pkgLenth);
						return;
					}

					std::string msgData(packet->MsgBegin(), packet->pkgLenth);

					if (packet->dealType == EMMsgDeal::Req)
					{
						ServerMessage::GetMessageHandle()->MsgHandle(world, channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Ret)
					{
						ServerMessage::GetMessageHandle()->MsgRetHandle(world, channel, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Redir)
					{
						ServerMessage::GetMessageHandle()->MsgRedirectHandle(world, channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Res)
					{
						ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

						if (MsgTask* task = proxyHelper->GetMsg(packet->msgId)) //client sock request
						{
							proxyHelper->DelMsg(packet->msgId);

							if (Message* message = task->GetMessage())
							{
								if (!message->ParseFromString(msgData))
								{
									task->SetFlag(EMTaskFlag::PaserError);
								}

							}

							task->Resume();
						}
						else
						{
							LoggerPrint::Log(world, EL10nCode_MsgFind);
						}
					}
					else
					{
						LoggerPrint::Log(world, EL10nCode_MsgDealType);
					}
				};

		}

		return;
	}

	void HandleServerShutdown()
	{
		if (ServerProxyHelper::CVPtr proxy = GetServerProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;

			proxy->ClearMsgMap();
		}

		if (ClientProxyHelper::CVPtr proxy = GetClientProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;

			proxy->ClearMsgMap();
		}

		return;
	}

	void HandleClientRegist()
	{
		ServerMessage::GetMessageHandle()->pClientRegistFunc(GetSelf<Server>());
	}

};
