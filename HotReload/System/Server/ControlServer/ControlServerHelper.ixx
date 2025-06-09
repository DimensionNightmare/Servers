module;
export module ControlServerHelper;

export import ThirdParty.PbGen;
import DNServer;
import DNServerProxyHelper;
import ServerEntityManagerHelper;
import DllUtils;
import MessagePack;
import ECSW;
import MessageRegister;

#define FUNCPLACE(class, func) &class::func, #class"_"#func

export class ControlServerHelper : public DNServer
{
private:

	ControlServerHelper() = delete;
	~ControlServerHelper() = default;

	ControlServerHelper(const ControlServerHelper&) = delete;
	// void operator=(const ControlServerHelper&) = delete;

	ControlServerHelper(ControlServerHelper&&) = delete;
	ControlServerHelper& operator=(ControlServerHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<ControlServerHelper>;
	using CVPtr = const Ptr&;

	DNServerProxyHelper::Ptr GetServerProxy() 
	{
		DNServerProxyHelper::Ptr proxy = GetComponent<DNServerProxyHelper>(EMComponentType::DNServerProxy);
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

	
	int HandleServerInit(MessageRegister* msgHandle)
	{
		msgHandle->RegMsgHandle();

		if (DNServerProxy::CVPtr proxy = GetComponent<DNServerProxy>(EMComponentType::DNServerProxy))
		{
			proxy->onConnection = [this](DNSocketChannel::CVPtr channel)
				{
					DNServerProxyHelper::CVPtr proxyHelper = GetServerProxy();

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

						// not used
						if (ServerEntity::CVPtr entity = channel->getContextPtr<ServerEntity>())
						{
							ServerEntityManagerHelper::CVPtr entityMan = GetServerEntityManager();
							entityMan->RemoveEntity(entity->ID());
							channel->deleteContextPtr();
						}

					}
				};

			proxy->onMessage = [this,msgHandle](DNSocketChannel::CVPtr channel, hv::Buffer* buf)
				{
					DNServerProxyHelper::CVPtr proxyHelper = GetServerProxy();

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

		return true;
	}

	int HandleServerShutdown()
	{
		if (DNServerProxyHelper::CVPtr proxy = GetServerProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;

			proxy->MsgMapClear();
		}

		return true;
	}

};
