module;
export module GateServerInit;

import FuncHelper;
import GateMessage;
import DNTask;
import Logger;
import DllUtils;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNServer;
import MessagePack;
import DNServerProxyHelper;
import std.compat;

#define FUNCPLACE(func) #func, func

export int HandleGateServerInit(DNServer::Ptr dnServer)
{
	GateMessageHandle::RegMsgHandle();

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

					TickMainSpaceDll(serverSock, FUNCPLACE(&DNServerProxy::InitConnectedChannel),  channel);
				}
				else
				{
					proxy->GetLogger()->Record(EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());
					if (Entity::Ptr entity = channel->getContext<Entity>())
					{
						switch (entity->GetEntityType())
						{
							case EMEntityType::Server:
								proxy->ServerEntityCloseEvent(entity);
								break;
							case EMEntityType::Proxy:
								proxy->ProxyEntityCloseEvent(entity);
								break;
							default:
								break;

						}

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
					GateMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Ret)
				{
					GateMessageHandle::MsgRetHandle(channel, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Redir)
				{
					GateMessageHandle::MsgRedirectHandle(channel, packet.msgId, packet.msgHashId, msgData);
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

	if (DNClientProxy::Ptr proxy = dnServer->GetComponent<DNClientProxy>(EMComponentType::DNClientProxy))
	{
		DNClientProxy::WPtr clientProxy = proxy->GetSelfW<DNClientProxy>();
		
		proxy->onConnection = [clientProxy](const DNSocketProxy::Ptr& channel)
			{
				DNClientProxy::Ptr proxy = clientProxy.lock();

				if(!proxy){ return ;}

				const std::string& peeraddr = channel->peeraddr();

				if (channel->isConnected())
				{
					proxy->GetLogger()->Record(EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());

					channel->SetWorld(proxy->GetWorld());
					
					proxy->SetRegistEvent(&GateMessage::Evt_ReqRegistSrv);
					TickMainSpaceDll(proxy, FUNCPLACE(&DNClientProxy::InitConnectedChannel),  channel);
				}
				else
				{
					proxy->GetLogger()->Record(EL10nCode_SrvConnOff, peeraddr, channel->fd(), channel->id());
					if (proxy->EMRegistState() == EMRegistState::Registed)
					{
						proxy->EMRegistState() = EMRegistState::None;
					}

					proxy->RegistType() = 0;
				}

				if (proxy->isReconnect())
				{

				}
			};

		proxy->onMessage = [clientProxy](const DNSocketProxy::Ptr& channel, hv::Buffer* buf)
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
					GateMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Redir)
				{
					GateMessageHandle::MsgRedirectHandle(channel, packet.msgId, packet.msgHashId, msgData);
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

export int HandleGateServerShutdown(DNServer::Ptr server)
{
	if (DNServerProxy::Ptr proxy = dnServer->GetComponent<DNServerProxy>(EMComponentType::DNServerProxy))
	{
		proxy->onConnection = nullptr;
		proxy->onMessage = nullptr;

		proxy->MsgMapClear();
	}

	if (DNClientProxy::Ptr proxy = dnServer->GetComponent<DNClientProxy>(EMComponentType::DNClientProxy))
	{
		proxy->onConnection = nullptr;
		proxy->onMessage = nullptr;
		proxy->SetRegistEvent(nullptr);

		proxy->MsgMapClear();
	}

	return true;
}
