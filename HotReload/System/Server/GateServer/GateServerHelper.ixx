module;
export module GateServerHelper;

export import ThirdParty.PbGen;
import Server;
import ClientProxyHelper;
import ServerProxyHelper;
import ServerEntityManagerHelper;
import ProxyEntityManagerHelper;
import FuncHelper;
import DllUtils;
import MessagePack;
import ECSW;
import MessageRegister;
import ServerEntityHelper;
import ProxyEntityHelper;

#define FUNCPLACE(class, func) &class::func, #class"_"#func

export class GateServerHelper : public Server
{

private:

	GateServerHelper() = delete;
	~GateServerHelper() = default;

	GateServerHelper(const GateServerHelper&) = delete;
	// void operator=(const GateServerHelper&) = delete;

	GateServerHelper(GateServerHelper&&) = delete;
	GateServerHelper& operator=(GateServerHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<GateServerHelper>;
	using CVPtr = const Ptr&;

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

	ProxyEntityManagerHelper::Ptr GetProxyEntityManager() 
	{
		ProxyEntityManagerHelper::Ptr proxy = GetComponent<ProxyEntityManagerHelper>(EMComponentType::ProxyEntityManager);
		if(!proxy || proxy->IsDisposed())
		{
			return nullptr;
		}

		return proxy;
	}

	/// @brief send close to change socket
	void ServerEntityCloseEvent(Entity::CVPtr entity)
	{
		// up to Global
		std::string binData;
		GMsg::g2G_RetRegistSrv request;
		request.set_server_id(entity->ID());
		request.set_is_regist(false);
		request.SerializeToString(&binData);
		MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, GetClientProxy()->GetChannel());

		GetServerEntityManager()->RemoveEntity(entity->ID());
	}

	void ProxyEntityCloseEvent(Entity::CVPtr entity)
	{
		ProxyEntityManagerHelper::CVPtr entityMan = GetProxyEntityManager();
		uint64_t entityId = entity->ID();

		ServerEntityHelper::Ptr serverEntity = nullptr;
		if (uint64_t serverId = entity->GetSelf<ProxyEntityHelper>()->RecordServerId())
		{
			serverEntity = GetServerEntityManager()->GetEntity(serverId);
		}

		if (serverEntity)
		{
			std::string binData;
			GMsg::g2L_RetProxyOffline request;
			request.set_entity_id(entityId);
			request.SerializeToString(&binData);
			MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, serverEntity->GetChannel());
		}

		entityMan->RemoveEntity(entityId);
	}

	int HandleServerInit(MessageRegister* msgHandle)
	{
		msgHandle->RegMsgHandle();

		if (ServerProxy::CVPtr proxy = GetServerProxy())
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

						TickMainSpaceDll(proxyHelper.get(), FUNCPLACE(ServerProxy,InitConnectedChannel),  channel);
					}
					else
					{
						GetLogger()->Record(EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());
						if (Entity::CVPtr entity = channel->getContextPtr<Entity>())
						{
							switch (entity->GetEntityType())
							{
								case EMEntityType::Server:
									ServerEntityCloseEvent(entity);
									break;
								case EMEntityType::Proxy:
									ProxyEntityCloseEvent(entity);
									break;
								default:
									break;

							}

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

						channel->SetWorld(pWorld);
						
						proxyHelper->SetRegistEvent(msgHandle->GetClientRegistFunc());
						TickMainSpaceDll(proxyHelper.get(), FUNCPLACE(ClientProxy,InitConnectedChannel),  channel);
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
