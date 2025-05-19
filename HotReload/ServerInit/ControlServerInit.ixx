module;
export module ControlServerInit;

import FuncHelper;
import ControlMessage;
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
import DNSocketProxy;
import ServerEntityManagerHelper;
import ControlServerHelper;
import ECSW;

#define FUNCPLACE(func) #func, func

export int HandleControlServerInit(const World::Ptr& world)
{
	static ControlMessageHandle MsgHandle;
	MsgHandle.RegMsgHandle();

	ControlServerHelper::Ptr dnServer = world->GetSystem<ControlServerHelper>(EMSystemType::DNServer);

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
					
					TickMainSpaceDll(proxy.get(), FUNCPLACE(&DNServerProxy::InitConnectedChannel),  channel);
				}
				else
				{
					proxy->GetLogger()->Record(EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());

					// channel->SetWorld(nullptr);

					// not used
					if (ServerEntity::Ptr entity = channel->getContextPtr<ServerEntity>())
					{
						ServerEntityManagerHelper::Ptr entityMan = proxy->GetOwner<ControlServerHelper>()->GetServerEntityManager();
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
