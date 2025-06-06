module;
export module LogicServerHelper;

export import ThirdParty.PbGen;
import DNClientProxyHelper;
import DNServerProxyHelper;
import RoomEntityManagerHelper;
import ClientEntityManagerHelper;
import MdbProxyHelper;
import DNServer;
import DllUtils;
import MessagePack;
import ECSW;
import MessageRegister;

#define FUNCPLACE(class, func) &class::func, #class"_"#func

export class LogicServerHelper : public DNServer
{

private:

	LogicServerHelper() = delete;
	~LogicServerHelper() = default;

	LogicServerHelper(const LogicServerHelper&) = delete;
	// void operator=(const LogicServerHelper&) = delete;

	LogicServerHelper(LogicServerHelper&&) = delete;
	LogicServerHelper& operator=(LogicServerHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<LogicServerHelper>;

	DNClientProxyHelper::Ptr GetClientProxy() { return GetComponent<DNClientProxyHelper>(EMComponentType::DNClientProxy); }

	DNServerProxyHelper::Ptr GetServerProxy() { return GetComponent<DNServerProxyHelper>(EMComponentType::DNServerProxy); }

	RoomEntityManagerHelper::Ptr GetRoomEntityManager() { return GetComponent<RoomEntityManagerHelper>(EMComponentType::RoomEntityManager); }

	ClientEntityManagerHelper::Ptr GetClientEntityManager() { return GetComponent<ClientEntityManagerHelper>(EMComponentType::ClientEntityManager); }

	MdbProxyHelper::Ptr GetMdbProxy(){ return GetComponent<MdbProxyHelper>(EMComponentType::MdbProxy); }

	bool InitDatabase()
	{
		if(MdbProxyHelper::Ptr proxy = GetMdbProxy())
		{
			try
			{
				World::Ptr pWorld = GetWorld();

				std::string* value = pWorld->LaunchParam("connection");

				auto connection = std::make_shared<sw::redis::Redis>(*value);
				connection->ping();

				proxy->AddConnection(std::move(connection));
			}
			catch (const std::exception& e)
			{
				GetLogger()->Record(ELogLevel_Debug, "{}", e.what());
				return false;
			}

			return true;
		}

		return false;
	}
	
	int HandleServerInit(MessageRegister* msgHandle)
	{
		msgHandle->RegMsgHandle();

		if (DNServerProxyHelper::Ptr proxy = GetServerProxy())
		{
			proxy->onConnection = [this](const DNSocketChannel::Ptr& channel)
				{
					DNServerProxyHelper::Ptr proxyHelper = GetServerProxy();

					if(!proxyHelper){ return ;}

					const std::string& peeraddr = channel->peeraddr();
					if (channel->isConnected())
					{
						GetLogger()->Record(EL10nCode_CliConnOn, peeraddr, channel->fd(), channel->id());

						channel->SetWorld(GetWorldW());
					}
					else
					{
						GetLogger()->Record(EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());
						if (RoomEntity::Ptr entity = channel->getContextPtr<RoomEntity>())
						{
							RoomEntityManagerHelper::Ptr entityMan = GetRoomEntityManager();
							entityMan->RemoveEntity(entity->ID());
							channel->deleteContextPtr();
						}
					}
				};

			proxy->onMessage = [this,msgHandle](const DNSocketChannel::Ptr& channel, hv::Buffer* buf)
				{
					DNServerProxyHelper::Ptr proxyHelper = GetServerProxy();

					if(!proxyHelper){ return ;}

					MessagePacket* packet = MessagePacket::From(buf->data());

					GetLogger()->Record(ELogLevel_Debug, "s {} Recv type={} With Mid:{}", channel->peeraddr(), static_cast<int>(packet->dealType), packet->msgId);

					if(packet->pkgLenth > 2 * 1024)
					{
						GetLogger()->Record(ELogLevel_Debug, "Recv byte len limit={}", packet->pkgLenth);
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
						if (DNTask<Message*>* task = proxyHelper->GetMsg(packet->msgId)) //client sock request
						{
							proxyHelper->DelMsg(packet->msgId);
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
							GetLogger()->Record(EL10nCode_MsgFind);
						}
					}
					else
					{
						GetLogger()->Record(EL10nCode_MsgDealType);
					}
				};

		}


		if (DNClientProxyHelper::Ptr proxy = GetClientProxy())
		{
			//client will re_create please check
			proxy->onConnection = [this,msgHandle](const DNSocketChannel::Ptr& channel)
				{
					DNClientProxyHelper::Ptr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}

					const std::string& peeraddr = channel->peeraddr();

					if (channel->isConnected())
					{
						GetLogger()->Record(EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());

						channel->SetWorld(GetWorldW());
						
						proxyHelper->SetRegistEvent(msgHandle->GetClientRegistFunc());
						TickMainSpaceDll(proxyHelper.get(), FUNCPLACE(DNClientProxy,InitConnectedChannel),  channel);

						GetClientEntityManager()->InitSqlConn(proxyHelper->GetSelf<DNClientProxy>());
					}
					else
					{
						GetLogger()->Record(EL10nCode_SrvConnOff, peeraddr, channel->fd(), channel->id());

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
								GetLogger()->Record(ELogLevel_Debug, "orgin not match peeraddr {} reclient ~", origin);
								proxyHelper->Timer()->setTimeout(200, [this, originIp, originPort](uint64_t timerID)
									{
										DNClientProxyHelper::Ptr proxyHelper = GetClientProxy();

										if(!proxyHelper){ return ;}
										TickMainSpaceDll(proxyHelper.get(), FUNCPLACE(DNClientProxy,RedirectClient),  std::stoi(originPort), originIp);

									});
							}
						}

						proxyHelper->SetRegistType(0);
					}

					if (proxyHelper->isReconnect())
					{
					}
				};

			proxy->onMessage = [this,msgHandle](const DNSocketChannel::Ptr& channel, hv::Buffer* buf)
				{
					DNClientProxyHelper::Ptr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}

					MessagePacket* packet = MessagePacket::From(buf->data());

					GetLogger()->Record(ELogLevel_Debug, "c {} Recv type={} With Mid:{}", channel->peeraddr(), static_cast<int>(packet->dealType), packet->msgId);

					if(packet->pkgLenth > 2 * 1024)
					{
						GetLogger()->Record(ELogLevel_Debug, "Recv byte len limit={}", packet->pkgLenth);
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
						if (DNTask<Message*>* task = proxyHelper->GetMsg(packet->msgId)) //client sock request
						{
							proxyHelper->DelMsg(packet->msgId);
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
							GetLogger()->Record(EL10nCode_MsgFind);
						}
					}
					else
					{
						GetLogger()->Record(EL10nCode_MsgDealType);
					}
				};

		}

		return InitDatabase();
	}

	int HandleServerShutdown()
	{
		if (DNServerProxyHelper::Ptr proxy = GetServerProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;

			proxy->MsgMapClear();
		}

		if (DNClientProxyHelper::Ptr proxy = GetClientProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;
			proxy->SetRegistEvent(nullptr);

			proxy->MsgMapClear();
		}

		if(MdbProxyHelper::Ptr mdbProxy = GetMdbProxy())
		{
			mdbProxy->ClearConnections();
		}

		return true;
	}

};
