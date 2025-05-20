module;
export module AuthServerInit;

import FuncHelper;
import AuthMessage;
import DNTask;
import Logger;
import DllUtils;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNServer;
import DNWebProxyHelper;
import MessagePack;
import std.compat;
import DNWebProxy;
import DNClientProxy;
import DNClientProxyHelper;
import DNServerProxyHelper;
import ECSW;
import AuthServerHelper;

#define FUNCPLACE(class, func) &class::func, #class"_"#func

export int HandleAuthServerInit(const World::Ptr& world)
{
	static AuthMessageHandle MsgHandle;

	static World* pWorld = world.get();

	AuthServerHelper::Ptr dnServer = world->GetSystem<AuthServerHelper>(EMSystemType::DNServer);

	if (DNWebProxy::Ptr proxy = dnServer->GetComponent<DNWebProxy>(EMComponentType::DNWebProxy))
	{
		hv::HttpService* service = new hv::HttpService();

		MsgHandle.RegApiHandle(dnServer->GetSelfW<DNServer>(), service);

		proxy->registerHttpService(service);
	}

	if (DNClientProxy::Ptr proxy = dnServer->GetComponent<DNClientProxy>(EMComponentType::DNClientProxy))
	{
		DNClientProxy::WPtr clientProxy = proxy->GetSelfW<DNClientProxy>();
		
		proxy->onConnection = [clientProxy](const DNSocketChannel::Ptr& channel)
			{
				DNClientProxy::Ptr proxy = clientProxy.lock();

				if(!proxy){ return ;}

				const std::string& peeraddr = channel->peeraddr();

				DNClientProxyHelper::Ptr proxyHelper = proxy->GetSelf<DNClientProxyHelper>();

				if (channel->isConnected())
				{
					proxy->GetLogger()->Record(EL10nCode_CliConnOn, peeraddr, channel->fd(), channel->id());

					channel->SetWorld(pWorld);

					proxyHelper->SetRegistEvent(&AuthMessage::Evt_ReqRegistSrv);
					TickMainSpaceDll(proxy.get(), FUNCPLACE(DNClientProxy,InitConnectedChannel),  channel);
				}
				else
				{
					proxy->GetLogger()->Record(EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());

					if (proxyHelper->GetRegistState() == EMRegistState::Registed)
					{
						proxyHelper->SetRegistState(EMRegistState::None);
					}

					proxy->SetRegistType(0);
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

				if (packet->dealType == EMMsgDeal::Res)
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

export int HandleAuthServerShutdown(const World::Ptr& world)
{
	AuthServerHelper::Ptr dnServer = world->GetSystem<AuthServerHelper>(EMSystemType::DNServer);
	
	if (DNClientProxyHelper::Ptr proxy = dnServer->GetComponent<DNClientProxyHelper>(EMComponentType::DNClientProxy))
	{
		proxy->onConnection = nullptr;
		proxy->onMessage = nullptr;
		proxy->SetRegistEvent(nullptr);

		// web use clientMsg
		proxy->MsgMapClear();
	}

	if (DNWebProxy::Ptr proxy = dnServer->GetComponent<DNWebProxy>(EMComponentType::DNWebProxy))
	{
		if (proxy->service)
		{
			hv::HttpService* temp = proxy->service;
			proxy->service = nullptr;
			delete temp;
		}
	}

	return true;
}
