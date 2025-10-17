export module LogicServerHelper;

import ThirdParty.PbGen;
import ClientProxyHelper;
import ServerProxyHelper;
import RoomEntityManagerHelper;
import ClientEntityManagerHelper;
import MdbProxyHelper;
import Server;
import MessagePack;
import ECSW;
import MessageRegister;
import FuncUtils;
import Logger;

export class LogicServerHelper : public Helper<LogicServerHelper, Server>
{

private:

	LogicServerHelper() = delete;
	~LogicServerHelper() = default;

public:

	ClientProxyHelper::Ptr GetClientProxy()
	{ 
		ClientProxyHelper::Ptr proxy = GetComponent<ClientProxyHelper>(EMComponentType::ClientProxy);
		if(!proxy || proxy->IsDisposed())
		{
			return nullptr;
		}
		return proxy;
	}

	ServerProxyHelper::Ptr GetServerProxy() 
	{
		ServerProxyHelper::Ptr proxy = GetComponent<ServerProxyHelper>(EMComponentType::ServerProxy);
		if(!proxy || proxy->IsDisposed())
		{
			return nullptr;
		}
		return proxy;
	}

	RoomEntityManagerHelper::Ptr GetRoomEntityManager() 
	{
		RoomEntityManagerHelper::Ptr proxy = GetComponent<RoomEntityManagerHelper>(EMComponentType::RoomEntityManager);
		if(!proxy || proxy->IsDisposed())
		{
			return nullptr;
		}
		return proxy;
	}

	ClientEntityManagerHelper::Ptr GetClientEntityManager() 
	{
		ClientEntityManagerHelper::Ptr proxy = GetComponent<ClientEntityManagerHelper>(EMComponentType::ClientEntityManager);
		if(!proxy || proxy->IsDisposed())
		{
			return nullptr;
		}
		return proxy;
	}
	
	MdbProxyHelper::Ptr GetMdbProxy() 
	{
		MdbProxyHelper::Ptr proxy = GetComponent<MdbProxyHelper>(EMComponentType::MdbProxy);
		if(!proxy || proxy->IsDisposed())
		{
			return nullptr;
		}
		return proxy;
	}

	int HandleServerInit(MessageRegister* msgHandle)
	{
		msgHandle->RegMsgHandle();

		if (ServerProxyHelper::CVPtr proxy = GetServerProxy())
		{
			proxy->onConnection = [this](SocketChannel::CVPtr channel)
				{
					ServerProxyHelper::CVPtr proxyHelper = GetServerProxy();

					if(!proxyHelper){ return ;}

					const std::string& peeraddr = channel->peeraddr();
					if (channel->isConnected())
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_CliConnOn, peeraddr, channel->fd(), channel->id());

						channel->SetWorld(GetWorldW());
					}
					else
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());
						if (RoomEntity::CVPtr entity = channel->getContextPtr<RoomEntity>())
						{
							RoomEntityManagerHelper::CVPtr entityMan = GetRoomEntityManager();
							entityMan->RemoveEntity(entity->ID());
							channel->deleteContextPtr();
						}
					}
				};

			proxy->onMessage = [this,msgHandle](SocketChannel::CVPtr channel, hv::Buffer* buf)
				{
					ServerProxyHelper::CVPtr proxyHelper = GetServerProxy();

					if(!proxyHelper){ return ;}

					MessagePacket* packet = MessagePacket::From(buf->data());

					LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "s {} Recv type={} With Mid:{}", channel->peeraddr(), static_cast<int>(packet->dealType), packet->msgId);

					if(packet->pkgLenth > 2 * 1024)
					{
						LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Recv byte len limit={}", packet->pkgLenth);
						return;
					}
					
					std::string msgData(packet->MsgBegin(), packet->pkgLenth);

					if (packet->dealType == EMMsgDeal::Req)
					{
						msgHandle->MsgHandle(channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Ret)
					{
						msgHandle->MsgRetHandle(channel, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Redir)
					{
						msgHandle->MsgRedirectHandle(channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Res)
					{
						if (Task<Message*>* task = proxyHelper->GetMsg(packet->msgId)) //client sock request
						{
							proxyHelper->DelMsg(packet->msgId);
							task->Resume();

							if (Message* message = task->GetResult())
							{
								if (!message->ParseFromString(msgData))
								{
									task->SetFlag(EMTaskFlag::PaserError);
								}

							}

							task->CallResume();
						}
						else
						{
							LoggerPrint::Log(GetWorld(), EL10nCode_MsgFind);
						}
					}
					else
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_MsgDealType);
					}
				};

		}


		if (ClientProxyHelper::CVPtr proxy = GetClientProxy())
		{
			//client will re_create please check
			proxy->onConnection = [this,msgHandle](SocketChannel::CVPtr channel)
				{
					ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}

					const std::string& peeraddr = channel->peeraddr();

					if (channel->isConnected())
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());

						channel->SetWorld(GetWorldW());
						
						proxyHelper->SetRegistEvent(msgHandle->GetClientRegistFunc());
						proxyHelper->InitConnectedChannel(channel);

						GetClientEntityManager()->InitSqlConn(proxyHelper->GetSelf<ClientProxy>());
					}
					else
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_SrvConnOff, peeraddr, channel->fd(), channel->id());

						std::string originIp;
						if(std::string* param = GetWorld()->LaunchParam("ctlIp"))
						{
							originIp = *param;
						}

						std::string originPort;
						if(std::string* param = GetWorld()->LaunchParam("ctlPort"))
						{
							originPort = *param;
						}

						std::string origin = std::format("{}:{}", originIp, originPort);
						if (proxyHelper->GetRegistState() == EMRegistState::Registed || peeraddr != origin)
						{
							proxyHelper->SetRegistState(EMRegistState::None);

							if (proxyHelper->isConnected())
							{
								LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "orgin not match peeraddr {} reclient ~", origin);
								proxyHelper->GetTimer()->SetTimeout(200, [this, originIp, originPort](uint64_t timerID)
									{
										ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

										if(!proxyHelper){ return ;}
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

			proxy->onMessage = [this,msgHandle](SocketChannel::CVPtr channel, hv::Buffer* buf)
				{
					ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}

					MessagePacket* packet = MessagePacket::From(buf->data());

					LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "c {} Recv type={} With Mid:{}", channel->peeraddr(), static_cast<int>(packet->dealType), packet->msgId);

					if(packet->pkgLenth > 2 * 1024)
					{
						LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Recv byte len limit={}", packet->pkgLenth);
						return;
					}

					std::string msgData(packet->MsgBegin(), packet->pkgLenth);

					if (packet->dealType == EMMsgDeal::Req)
					{
						msgHandle->MsgHandle(channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Ret)
					{
						msgHandle->MsgRetHandle(channel, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Redir)
					{
						msgHandle->MsgRedirectHandle(channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Res)
					{
						if (Task<Message*>* task = proxyHelper->GetMsg(packet->msgId)) //client sock request
						{
							proxyHelper->DelMsg(packet->msgId);
							task->Resume();

							if (Message* message = task->GetResult())
							{
								if (!message->ParseFromString(msgData))
								{
									task->SetFlag(EMTaskFlag::PaserError);
								}

							}

							task->CallResume();
						}
						else
						{
							LoggerPrint::Log(GetWorld(), EL10nCode_MsgFind);
						}
					}
					else
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_MsgDealType);
					}
				};

		}

		return true;
	}

	int HandleServerShutdown()
	{
		if (ServerProxyHelper::CVPtr proxy = GetServerProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;

			proxy->MsgMapClear();
		}

		if (ClientProxyHelper::CVPtr proxy = GetClientProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;
			proxy->SetRegistEvent(nullptr);

			proxy->MsgMapClear();
		}

		return true;
	}

};
