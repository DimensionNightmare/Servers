module;
export module GlobalServerInit;

import FuncHelper;
import GlobalMessage;
import DNTask;
import Logger;
import DllUtils;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNServer;
import DNServerProxyHelper;
import MessagePack;
import std.compat;
import DNServerProxy;
import ServerEntity;
import ServerEntityManagerHelper;
import GlobalServerHelper;
import ECSW;

#define FUNCPLACE(func) #func, func

export int HandleGlobalServerInit(const World::Ptr& world)
{
	static GlobalMessageHandle MsgHandle;
	MsgHandle.RegMsgHandle();

	GlobalServerHelper::Ptr dnServer = world->GetSystem<GlobalServerHelper>(EMSystemType::DNServer);

	if (DNServerProxy::Ptr proxy = dnServer->GetComponent<DNServerProxy>(EMComponentType::DNServerProxy))
	{
		DNServerProxy::WPtr serverProxy = proxy->GetSelfW<DNServerProxy>();

		proxy->onConnection = [serverProxy](const DNSocketProxy::Ptr& channel)
			{
				DNServerProxy::Ptr proxy = serverProxy.lock();

				if(!proxy){ return ;}

				const std::string& peeraddr = channel->peeraddr();
				if (channel->isConnected())
				{
					proxy->GetLogger()->Record(EL10nCode_CliConnOn, peeraddr, channel->fd(), channel->id());

					// channel->SetWorld(proxy->GetOwner()->GetWorld());

					DNServerProxy::Ptr serverProxy = proxy->GetOwner()->GetComponent<DNServerProxy>(EMComponentType::DNServerProxy);

					TickMainSpaceDll(serverProxy.get(), FUNCPLACE(&DNServerProxy::InitConnectedChannel),  channel);
				}
				else
				{
					// channel->SetWorld(nullptr);

					proxy->GetLogger()->Record(EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());

					if (ServerEntity::Ptr entity = channel->getContextPtr<ServerEntity>())
					{
						ServerEntityManagerHelper::Ptr entityMan = proxy->GetOwner<GlobalServerHelper>()->GetServerEntityManager();
						entityMan->RemoveEntity(entity->ID());
						channel->setContextPtr(nullptr);
					}
				}
			};

		proxy->onMessage = [serverProxy, world](const DNSocketProxy::Ptr& channel, hv::Buffer* buf)
			{
				DNServerProxy::Ptr proxy = serverProxy.lock();

				if(!proxy){ return ;}

				MessagePacket packet;
				memcpy(&packet, buf->data(), MessagePacket::PackLenth);

				proxy->GetLogger()->Record(ELogLevel_Debug, "s {} Recv type={} With Mid:{}", channel->peeraddr(), static_cast<int>(packet.dealType), packet.msgId);

				if(packet.pkgLenth > 2 * 1024)
				{
					proxy->GetLogger()->Record(ELogLevel_Debug, "Recv byte len limit={}", packet.pkgLenth);
					return;
				}

				std::string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);

				if (packet.dealType == EMMsgDeal::Req)
				{
					MsgHandle.MsgHandle(world, channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Ret)
				{
					MsgHandle.MsgRetHandle(world, channel, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Redir)
				{
					MsgHandle.MsgRedirectHandle(world, channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Res)
				{
					DNServerProxyHelper::Ptr proxyHelper = proxy->GetSelf<DNServerProxyHelper>();

					if (DNTask<Message*>* task = proxyHelper->GetMsg(packet.msgId)) //client sock request
					{
						proxyHelper->DelMsg(packet.msgId);
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

	if (DNClientProxy::Ptr proxy = dnServer->GetComponent<DNClientProxy>(EMComponentType::DNClientProxy))
	{
		DNClientProxy::WPtr clientProxy = proxy->GetSelfW<DNClientProxy>();
		
		proxy->onConnection = [clientProxy](const DNSocketProxy::Ptr& channel)
			{
				DNClientProxy::Ptr proxy = clientProxy.lock();

				if(!proxy){ return ;}

				DNClientProxyHelper::Ptr proxyHelper = proxy->GetSelf<DNClientProxyHelper>();

				const std::string& peeraddr = channel->peeraddr();

				if (channel->isConnected())
				{
					proxy->GetLogger()->Record(EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());

					proxyHelper->SetRegistEvent(&GlobalMessage::Evt_ReqRegistSrv);

					TickMainSpaceDll(proxy.get(), FUNCPLACE(&DNClientProxy::InitConnectedChannel),  channel);
				}
				else
				{
					proxy->GetLogger()->Record(EL10nCode_SrvConnOff, peeraddr, channel->fd(), channel->id());
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

		proxy->onMessage = [clientProxy, world](const DNSocketProxy::Ptr& channel, hv::Buffer* buf)
			{
				DNClientProxy::Ptr proxy = clientProxy.lock();

				if(!proxy){ return ;}

				MessagePacket packet;
				memcpy(&packet, buf->data(), MessagePacket::PackLenth);

				proxy->GetLogger()->Record(ELogLevel_Debug, "c {} Recv type={} With Mid:{}", channel->peeraddr(), static_cast<int>(packet.dealType), packet.msgId);

				if(packet.pkgLenth > 2 * 1024)
				{
					proxy->GetLogger()->Record(ELogLevel_Debug, "Recv byte len limit={}", packet.pkgLenth);
					return;
				}
				
				std::string msgData(buf->base + MessagePacket::PackLenth, packet.pkgLenth);

				if (packet.dealType == EMMsgDeal::Req)
				{
					MsgHandle.MsgHandle(world, channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Redir)
				{
					MsgHandle.MsgRedirectHandle(world, channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Res)
				{
					DNClientProxyHelper::Ptr proxyHelper = proxy->GetSelf<DNClientProxyHelper>();

					if (DNTask<Message*>* task = proxyHelper->GetMsg(packet.msgId)) //client sock request
					{
						proxyHelper->DelMsg(packet.msgId);
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

export int HandleGlobalServerShutdown(const World::Ptr& world)
{
	GlobalServerHelper::Ptr dnServer = world->GetSystem<GlobalServerHelper>(EMSystemType::DNServer);

	if (DNServerProxyHelper::Ptr serverSock = dnServer->GetServerProxy())
	{
		serverSock->onConnection = nullptr;
		serverSock->onMessage = nullptr;

		serverSock->MsgMapClear();
	}

	if (DNClientProxyHelper::Ptr clientSock = dnServer->GetClientProxy())
	{
		clientSock->onConnection = nullptr;
		clientSock->onMessage = nullptr;
		clientSock->SetRegistEvent(nullptr);

		clientSock->MsgMapClear();
	}

	return true;
}
