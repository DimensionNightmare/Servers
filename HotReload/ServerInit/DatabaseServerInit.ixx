module;
export module DatabaseServerInit;

import FuncHelper;
import DatabaseMessage;
import DNTask;
import Logger;
import DllUtils;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNServer;
import DNClientProxyHelper;
import MessagePack;
import std.compat;
import DNClientProxy;
import ECSW;
import DatabaseServerHelper;

#define FUNCPLACE(func) #func, func

export int HandleDatabaseServerInit(const World::Ptr& world)
{
	static DatabaseMessageHandle MsgHandle;
	MsgHandle.RegMsgHandle();

	DatabaseServerHelper::Ptr dnServer = world->GetSystem<DatabaseServerHelper>(EMSystemType::DNServer);

	if (DNClientProxy::Ptr proxy = dnServer->GetComponent<DNClientProxy>(EMComponentType::DNClientProxy))
	{
		DNClientProxy::WPtr clientProxy = proxy->GetSelfW<DNClientProxy>();
		
		proxy->onConnection = [clientProxy](const DNSocketProxy::Ptr& channel)
			{
				DNClientProxy::Ptr proxy = clientProxy.lock();

				if(!proxy){ return ;}

				const std::string& peeraddr = channel->peeraddr();

				DNClientProxyHelper::Ptr proxyHelper = proxy->GetSelf<DNClientProxyHelper>();

				if (channel->isConnected())
				{
					proxy->GetLogger()->Record(EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());
					
					proxyHelper->SetRegistEvent(&DatabaseMessage::Evt_ReqRegistSrv);
					TickMainSpaceDll(proxy.get(), FUNCPLACE(&DNClientProxy::InitConnectedChannel),  channel);
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

						if (proxy->isConnected())
						{
							proxy->GetLogger()->Record(ELogLevel_Debug, "orgin not match peeraddr {} reclient ~", origin);

							proxy->Timer()->setTimeout(200, [clientProxy, originIp, originPort](uint64_t timerID)
								{
									DNClientProxy::Ptr proxy = clientProxy.lock();
									if(!proxy){ return ;}
									TickMainSpaceDll(proxy.get(), FUNCPLACE(&DNClientProxy::RedirectClient),  std::stoi(originPort), originIp);
								});
						}
					}

					proxy->SetRegistType(0);
				}

				if (proxy->isReconnect())
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
				else if (packet.dealType == EMMsgDeal::Ret)
				{
					MsgHandle.MsgRetHandle(world, channel, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Res)
				{
					DNClientProxyHelper::Ptr proxyHelper = proxy->GetSelf<DNClientProxyHelper>();

					if (DNTask<Message*>* task = proxyHelper->GetMsg(packet.msgId)) // client sock request
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

	return dnServer->InitDatabase();
}

export int HandleDatabaseServerShutdown(const World::Ptr& world)
{
	DatabaseServerHelper::Ptr dnServer = world->GetSystem<DatabaseServerHelper>(EMSystemType::DNServer);
	
	if (DNClientProxyHelper::Ptr proxy = dnServer->GetComponent<DNClientProxyHelper>(EMComponentType::DNClientProxy))
	{
		proxy->onConnection = nullptr;
		proxy->onMessage = nullptr;
		proxy->SetRegistEvent(nullptr);

		proxy->MsgMapClear();
	}

	return true;
}
