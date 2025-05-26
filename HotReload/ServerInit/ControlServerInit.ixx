module;
export module ControlServerInit;

import ControlMessage;
import DllUtils;
import MessagePack;
import ControlServerHelper;
import ECSW;

#define FUNCPLACE(class, func) &class::func, #class"_"#func

export int HandleControlServerInit(const World::Ptr& world)
{
	static ControlMessageHandle MsgHandle;
	MsgHandle.RegMsgHandle();

	World::WPtr pWorld = world->GetSelfW<World>();

	ControlServerHelper::Ptr dnServer = world->GetSystem<ControlServerHelper>(EMSystemType::DNServer);

	if (DNServerProxy::Ptr proxy = dnServer->GetComponent<DNServerProxy>(EMComponentType::DNServerProxy))
	{
		DNServerProxy::WPtr serverProxy = proxy->GetSelfW<DNServerProxy>();
	
		proxy->onConnection = [serverProxy,pWorld](const DNSocketChannel::Ptr& channel)
			{
				DNServerProxy::Ptr proxy = serverProxy.lock();

				if(!proxy){ return ;}

				const std::string& peeraddr = channel->peeraddr();
				if (channel->isConnected())
				{
					proxy->GetLogger()->Record(EL10nCode_CliConnOn, peeraddr, channel->fd(), channel->id());

					channel->SetWorld(pWorld);
					
					TickMainSpaceDll(proxy.get(), FUNCPLACE(DNServerProxy,InitConnectedChannel),  channel);
				}
				else
				{
					proxy->GetLogger()->Record(EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());

					// not used
					if (ServerEntity::Ptr entity = channel->getContextPtr<ServerEntity>())
					{
						ServerEntityManagerHelper::Ptr entityMan = proxy->GetOwner<ControlServerHelper>()->GetServerEntityManager();
						entityMan->RemoveEntity(entity->ID());
						channel->deleteContextPtr();
					}

				}
			};

		proxy->onMessage = [serverProxy](const DNSocketChannel::Ptr& channel, hv::Buffer* buf)
			{
				DNServerProxy::Ptr proxy = serverProxy.lock();

				if(!proxy){ return ;}
				
				MessagePacket* packet = MessagePacket::From(buf->data());

				proxy->GetLogger()->Record(ELogLevel_Debug, "s {} Recv type={} With Mid:{}", channel->peeraddr(), static_cast<int>(packet->dealType), packet->msgId);

				if(packet->pkgLenth > 2 * 1024)
				{
					proxy->GetLogger()->Record(ELogLevel_Debug, "Recv byte len limit={}", packet->pkgLenth);
					return;
				}
				
				std::string msgData(packet->MsgBegin(), packet->pkgLenth);

				if (packet->dealType == EMMsgDeal::Req)
				{
					MsgHandle.MsgHandle(channel, packet->msgId, packet->msgHashId, msgData);
				}
				else if (packet->dealType == EMMsgDeal::Ret)
				{
					MsgHandle.MsgRetHandle(channel, packet->msgHashId, msgData);
				}
				else if (packet->dealType == EMMsgDeal::Redir)
				{
					MsgHandle.MsgRedirectHandle(channel, packet->msgId, packet->msgHashId, msgData);
				}
				else if (packet->dealType == EMMsgDeal::Res)
				{
					DNServerProxyHelper::Ptr proxyHelper = proxy->GetSelf<DNServerProxyHelper>();

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
						proxy->GetLogger()->Record(EL10nCode_MsgFind);
					}
				}
				else
				{
					proxy->GetLogger()->Record(EL10nCode_MsgDealType);
				}
			};

	}

	return true;
}

export int HandleControlServerShutdown(const World::Ptr& world)
{
	ControlServerHelper::Ptr dnServer = world->GetSystem<ControlServerHelper>(EMSystemType::DNServer);
	
	if (DNServerProxyHelper::Ptr proxy = dnServer->GetComponent<DNServerProxyHelper>(EMComponentType::DNServerProxy))
	{
		proxy->onConnection = nullptr;
		proxy->onMessage = nullptr;

		proxy->MsgMapClear();
	}

	return true;
}
