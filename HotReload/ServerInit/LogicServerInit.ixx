module;
export module LogicServerInit;

import LogicMessage;
import DllUtils;
import MessagePack;
import LogicServerHelper;
import ECSW;

#define FUNCPLACE(class, func) &class::func, #class"_"#func

export int HandleLogicServerInit(const World::Ptr& world)
{
	static LogicMessageHandle MsgHandle;
	MsgHandle.RegMsgHandle();

	World::WPtr pWorld = world->GetSelfW<World>();

	LogicServerHelper::Ptr dnServer = world->GetSystem<LogicServerHelper>(EMSystemType::DNServer);

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
				}
				else
				{
					proxy->GetLogger()->Record(EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());
					if (RoomEntity::Ptr entity = channel->getContextPtr<RoomEntity>())
					{
						RoomEntityManagerHelper::Ptr entityMan = proxy->GetOwner<LogicServerHelper>()->GetRoomEntityManager();
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


	if (DNClientProxy::Ptr proxy = dnServer->GetComponent<DNClientProxy>(EMComponentType::DNClientProxy))
	{
		DNClientProxy::WPtr clientProxy = proxy->GetSelfW<DNClientProxy>();

		//client will re_create please check
		proxy->onConnection = [clientProxy,pWorld](const DNSocketChannel::Ptr& channel)
			{
				DNClientProxy::Ptr proxy = clientProxy.lock();

				if(!proxy){ return ;}

				const std::string& peeraddr = channel->peeraddr();

				DNClientProxyHelper::Ptr proxyHelper = proxy->GetSelf<DNClientProxyHelper>();

				if (channel->isConnected())
				{
					proxy->GetLogger()->Record(EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());

					channel->SetWorld(pWorld);
					
					proxyHelper->SetRegistEvent(&LogicMessage::Evt_ReqRegistSrv);
					TickMainSpaceDll(proxy.get(), FUNCPLACE(DNClientProxy,InitConnectedChannel),  channel);

					proxyHelper->GetOwner<LogicServerHelper>()->GetClientEntityManager()->InitSqlConn(proxy);
				}
				else
				{
					proxy->GetLogger()->Record(EL10nCode_SrvConnOff, peeraddr, channel->fd(), channel->id());

					std::string originIp;
					if(std::string* param = proxy->GetOwner()->GetWorld()->LaunchParam("ctlIp"))
					{
						originIp = *param;
					}

					std::string originPort;
					if(std::string* param = proxy->GetOwner()->GetWorld()->LaunchParam("ctlPort"))
					{
						originPort = *param;
					}

					std::string origin = std::format("{}:{}", originIp, originPort);
					if (proxyHelper->GetRegistState() == EMRegistState::Registed || peeraddr != origin)
					{
						proxyHelper->SetRegistState(EMRegistState::None);

						if (proxyHelper->isConnected())
						{
							proxy->GetLogger()->Record(ELogLevel_Debug, "orgin not match peeraddr {} reclient ~", origin);
							proxyHelper->Timer()->setTimeout(200, [clientProxy, originIp, originPort](uint64_t timerID)
								{
									DNClientProxy::Ptr proxy = clientProxy.lock();
									if(!proxy){ return ;}
									TickMainSpaceDll(proxy.get(), FUNCPLACE(DNClientProxy,RedirectClient),  std::stoi(originPort), originIp);

								});
						}
					}

					proxyHelper->SetRegistType(0);
				}

				if (proxy->isReconnect())
				{
				}
			};

		proxy->onMessage = [clientProxy](const DNSocketChannel::Ptr& channel, hv::Buffer* buf)
			{
				DNClientProxy::Ptr proxy = clientProxy.lock();

				if(!proxy){ return ;}

				MessagePacket* packet = MessagePacket::From(buf->data());

				proxy->GetLogger()->Record(ELogLevel_Debug, "c {} Recv type={} With Mid:{}", channel->peeraddr(), static_cast<int>(packet->dealType), packet->msgId);

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
					DNClientProxyHelper::Ptr proxyHelper = proxy->GetSelf<DNClientProxyHelper>();

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

	return dnServer->InitDatabase();
}

export int HandleLogicServerShutdown(const World::Ptr& world)
{
	LogicServerHelper::Ptr dnServer = world->GetSystem<LogicServerHelper>(EMSystemType::DNServer);

	if (DNServerProxyHelper::Ptr proxy = dnServer->GetServerProxy())
	{
		proxy->onConnection = nullptr;
		proxy->onMessage = nullptr;

		proxy->MsgMapClear();
	}

	if (DNClientProxyHelper::Ptr proxy = dnServer->GetClientProxy())
	{
		proxy->onConnection = nullptr;
		proxy->onMessage = nullptr;
		proxy->SetRegistEvent(nullptr);

		proxy->MsgMapClear();
	}

	if(auto mdbProxy = dnServer->GetMdbProxy())
	{
		mdbProxy->ClearConnections();
	}

	return true;
}
