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

#define FUNCPLACE(func) #func, func

export int HandleControlServerInit(DNServer::Ptr server)
{
	ControlMessageHandle::RegMsgHandle();

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

					channel->SetWorld(proxy->GetWorld());
					
					TickMainSpaceDll(proxy, FUNCPLACE(&DNServerProxy::InitConnectedChannel),  channel);
				}
				else
				{
					proxy->GetLogger()->Record(EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());

					// not used
					if (ServerEntity::Ptr entity = channel->getContext<ServerEntity>())
					{
						ServerEntityManagerHelper* entityMan = serverProxy->GetServerEntityManager();
						entityMan->RemoveEntity(entity->ID());
						channel->setContext(nullptr);
					}

				}
			};

		proxy->onMessage = [serverProxy](const DNSocketProxy::Ptr& channel, hv::Buffer* buf)
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
					ControlMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Ret)
				{
					ControlMessageHandle::MsgRetHandle(channel, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Redir)
				{
					ControlMessageHandle::MsgRedirectHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Res)
				{
					if (DNTask<Message*>* task = proxy->GetMsg(packet.msgId)) //client sock request
					{
						proxy->DelMsg(packet.msgId);
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

export int HandleControlServerShutdown(DNServer::Ptr server)
{
	
	if (DNServerProxy::Ptr proxy = dnServer->GetComponent<DNServerProxy>(EMComponentType::DNServerProxy))
	{
		proxy->onConnection = nullptr;
		proxy->onMessage = nullptr;

		proxy->MsgMapClear();
	}

	return true;
}
