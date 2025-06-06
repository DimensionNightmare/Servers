module;
export module GlobalServerHelper;

export import ThirdParty.PbGen;
import DNClientProxyHelper;
import DNServerProxyHelper;
import ServerEntityManagerHelper;
import DllUtils;
import FuncHelper;
import DNServer;
import MessagePack;
import ECSW;
import MessageRegister;

#define FUNCPLACE(class, func) &class::func, #class"_"#func

export class GlobalServerHelper : public DNServer
{

private:

	GlobalServerHelper() = delete;
	~GlobalServerHelper() = default;

	GlobalServerHelper(const GlobalServerHelper&) = delete;
	// void operator=(const GlobalServerHelper&) = delete;

	GlobalServerHelper(GlobalServerHelper&&) = delete;
	GlobalServerHelper& operator=(GlobalServerHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<GlobalServerHelper>;

	DNClientProxyHelper::Ptr GetClientProxy() { return GetComponent<DNClientProxyHelper>(EMComponentType::DNClientProxy); }

	DNServerProxyHelper::Ptr GetServerProxy() { return GetComponent<DNServerProxyHelper>(EMComponentType::DNServerProxy); }

	ServerEntityManagerHelper::Ptr GetServerEntityManager() { return GetComponent<ServerEntityManagerHelper>(EMComponentType::ServerEntityManager); }

	void UpdateServerGroup()
	{
		ServerEntityManagerHelper::Ptr entityMan = GetServerEntityManager();

		std::list<ServerEntity::Ptr>& gates = entityMan->GetEntitysByType(EMServerType::GateServer);
		if (gates.empty())
		{
			return;
		}

		std::list<ServerEntity::Ptr>& dbs = entityMan->GetEntitysByType(EMServerType::DatabaseServer);
		std::list<ServerEntity::Ptr>& logics = entityMan->GetEntitysByType(EMServerType::LogicServer);

		// alloc gate
		GMsg::COM_RetChangeCtlSrv request;
		std::string binData;

		auto registControl = [&](const ServerEntity::Ptr& beEntity, const ServerEntity::Ptr& entity) ->bool
		{
			ServerEntityHelper::Ptr entityHelper = entity->GetSelf<ServerEntityHelper>();

			const DNSocketChannel::Ptr& channel = entityHelper->GetChannel();
			if(!channel)
			{
				return false;
			}
			entityHelper->SetLinkNode(beEntity);

			channel->deleteContextPtr();

			ServerEntityHelper::Ptr beEntityHelper = beEntity->GetSelf<ServerEntityHelper>();

			// sendData
			request.set_server_ip(beEntityHelper->ServerIp());
			request.set_server_port(beEntityHelper->ServerPort());

			request.SerializeToString(&binData);
			// timer destory
			entityHelper->SetTimerId(TickMainSpaceDll(entityHelper.get(), FUNCPLACE(ServerEntityManager,CheckEntityCloseTimer), entityHelper->ID()));
			MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, channel);
			entityHelper->SetChannel(nullptr);

			return true;
		};

		for (ServerEntity::Ptr gate : gates)
		{
			if (gate->HasFlag(EMServerEntityFlag::Locked))
			{
				continue;
			}

			std::list<ServerEntity::Ptr>& gatesDb = gate->GetMapLinkNode(EMServerType::DatabaseServer);
			std::list<ServerEntity::Ptr>& gatesLogic = gate->GetMapLinkNode(EMServerType::LogicServer);
			if (!dbs.empty() && gatesDb.size() < 1)
			{
				ServerEntity::Ptr ele = dbs.front();
				// dbs.pop_front();
				if(registControl(gate, ele))
				{
					entityMan->UnMountEntity(ele->GetSelf<ServerEntityHelper>());
					gatesDb.emplace_back(ele);
				}
			}

			if (!logics.empty() && gatesLogic.size() < 1)
			{
				ServerEntity::Ptr ele = logics.front();
				// logics.pop_front();
				if(registControl(gate, ele))
				{
					entityMan->UnMountEntity(ele->GetSelf<ServerEntityHelper>());
					gatesLogic.emplace_back(ele);
				}
			}

			if (gatesDb.size() && gatesLogic.size())
			{
				// UnMountEntity(gate->GetServerType(), it);
				gate->SetFlag(EMServerEntityFlag::Locked);
				GetLogger()->Record(ELogLevel_Debug, "Gate:{} locked!", gate->ID());
			}

		}
	}

	int HandleServerInit(MessageRegister* msgHandle)
	{
		msgHandle->RegMsgHandle();

		if (DNServerProxy::Ptr proxy = GetComponent<DNServerProxy>(EMComponentType::DNServerProxy))
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

						TickMainSpaceDll(proxyHelper.get(), FUNCPLACE(DNServerProxy,InitConnectedChannel),  channel);
					}
					else
					{
						GetLogger()->Record(EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());

						if (ServerEntity::Ptr entity = channel->getContextPtr<ServerEntity>())
						{
							ServerEntityManagerHelper::Ptr entityMan = GetServerEntityManager();
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

		if (DNClientProxy::Ptr proxy = GetComponent<DNClientProxy>(EMComponentType::DNClientProxy))
		{
			proxy->onConnection = [this,msgHandle](const DNSocketChannel::Ptr& channel)
				{
					DNClientProxyHelper::Ptr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}

					const std::string& peeraddr = channel->peeraddr();

					if (channel->isConnected())
					{
						GetLogger()->Record(EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());

						channel->SetWorld(pWorld);

						proxyHelper->SetRegistEvent(msgHandle->GetClientRegistFunc());

						TickMainSpaceDll(proxyHelper.get(), FUNCPLACE(DNClientProxy,InitConnectedChannel),  channel);
					}
					else
					{
						GetLogger()->Record(EL10nCode_SrvConnOff, peeraddr, channel->fd(), channel->id());
						if (proxyHelper->GetRegistState() == EMRegistState::Registed)
						{
							proxyHelper->SetRegistState(EMRegistState::None);
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

		return true;

	}

	int HandleServerShutdown()
	{
		
		if (DNServerProxyHelper::Ptr serverSock = GetServerProxy())
		{
			serverSock->onConnection = nullptr;
			serverSock->onMessage = nullptr;

			serverSock->MsgMapClear();
		}

		if (DNClientProxyHelper::Ptr clientSock = GetClientProxy())
		{
			clientSock->onConnection = nullptr;
			clientSock->onMessage = nullptr;
			clientSock->SetRegistEvent(nullptr);

			clientSock->MsgMapClear();
		}

		return true;
	}

};
