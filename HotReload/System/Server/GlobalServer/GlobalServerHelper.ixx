export module GlobalServerHelper;

export import ThirdParty.PbGen;
import ClientProxyHelper;
import ServerProxyHelper;
import ServerEntityManagerHelper;
import FuncHelper;
import Server;
import MessagePack;
import ECSW;
import MessageRegister;
import FuncUtils;

export class GlobalServerHelper : public Helper<GlobalServerHelper, Server>
{

private:

	GlobalServerHelper() = delete;
	~GlobalServerHelper() = default;

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

	ServerEntityManagerHelper::Ptr GetServerEntityManager() 
	{
		ServerEntityManagerHelper::Ptr proxy = GetComponent<ServerEntityManagerHelper>(EMComponentType::ServerEntityManager);
		if(!proxy || proxy->IsDisposed())
		{
			return nullptr;
		}

		return proxy;
	}

	void UpdateServerGroup()
	{
		ServerEntityManagerHelper::CVPtr entityMan = GetServerEntityManager();

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

		auto registControl = [&](ServerEntityHelper::CVPtr beEntityHelper, ServerEntityHelper::CVPtr entityHelper) ->bool
		{
			SocketChannel::CVPtr channel = entityHelper->GetChannel();
			if(!channel)
			{
				return false;
			}
			entityHelper->SetLinkNode(beEntityHelper);

			channel->deleteContextPtr();

			// sendData
			request.set_server_ip(beEntityHelper->ServerIp());
			request.set_server_port(beEntityHelper->ServerPort());

			request.SerializeToString(&binData);
			// timer destory
			entityHelper->SetTimerId(entityMan->CheckEntityCloseTimer(entityHelper->ID()));
			MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, channel);
			entityHelper->SetChannel(nullptr);

			return true;
		};

		for (ServerEntity::Ptr& gate : gates)
		{
			if (gate->HasFlag(EMServerEntityFlag::Locked))
			{
				continue;
			}

			ServerEntityHelper::CVPtr gateHelper = gate->GetSelf<ServerEntityHelper>();

			std::list<ServerEntity::Ptr>& gatesDb = gate->GetMapLinkNode(EMServerType::DatabaseServer);
			std::list<ServerEntity::Ptr>& gatesLogic = gate->GetMapLinkNode(EMServerType::LogicServer);
			if (!dbs.empty() && gatesDb.size() < 1)
			{
				ServerEntityHelper::CVPtr dbHelper = dbs.front()->GetSelf<ServerEntityHelper>();
				// dbs.pop_front();
				if(registControl(gateHelper, dbHelper))
				{
					entityMan->UnMountEntity(dbHelper);
					gatesDb.emplace_back(dbHelper);
				}
			}

			if (!logics.empty() && gatesLogic.size() < 1)
			{
				ServerEntityHelper::CVPtr logicHelper = logics.front()->GetSelf<ServerEntityHelper>();
				// logics.pop_front();
				if(registControl(gateHelper, logicHelper))
				{
					entityMan->UnMountEntity(logicHelper);
					gatesLogic.emplace_back(logicHelper);
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

		if (ServerProxy::CVPtr proxy = GetComponent<ServerProxy>(EMComponentType::ServerProxy))
		{
			proxy->onConnection = [this](SocketChannel::CVPtr channel)
				{
					ServerProxyHelper::CVPtr proxyHelper = GetServerProxy();

					if(!proxyHelper){ return ;}

					const std::string& peeraddr = channel->peeraddr();
					if (channel->isConnected())
					{
						GetLogger()->Record(EL10nCode_CliConnOn, peeraddr, channel->fd(), channel->id());

						channel->SetWorld(GetWorldW());

						proxyHelper->InitConnectedChannel(channel);
					}
					else
					{
						GetLogger()->Record(EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());

						if (ServerEntity::CVPtr entity = channel->getContextPtr<ServerEntity>())
						{
							ServerEntityManagerHelper::CVPtr entityMan = GetServerEntityManager();
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
							GetLogger()->Record(EL10nCode_MsgFind);
						}
					}
					else
					{
						GetLogger()->Record(EL10nCode_MsgDealType);
					}
				};

		}

		if (ClientProxy::CVPtr proxy = GetComponent<ClientProxy>(EMComponentType::ClientProxy))
		{
			proxy->onConnection = [this,msgHandle](SocketChannel::CVPtr channel)
				{
					ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}

					const std::string& peeraddr = channel->peeraddr();

					if (channel->isConnected())
					{
						GetLogger()->Record(EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());

						channel->SetWorld(GetWorldW());

						proxyHelper->SetRegistEvent(msgHandle->GetClientRegistFunc());

						proxyHelper->InitConnectedChannel(channel);
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

			proxy->onMessage = [this,msgHandle](SocketChannel::CVPtr channel, hv::Buffer* buf)
				{
					ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

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
		
		if (ServerProxyHelper::CVPtr serverSock = GetServerProxy())
		{
			serverSock->onConnection = nullptr;
			serverSock->onMessage = nullptr;

			serverSock->MsgMapClear();
		}

		if (ClientProxyHelper::CVPtr clientSock = GetClientProxy())
		{
			clientSock->onConnection = nullptr;
			clientSock->onMessage = nullptr;
			clientSock->SetRegistEvent(nullptr);

			clientSock->MsgMapClear();
		}

		return true;
	}

};
