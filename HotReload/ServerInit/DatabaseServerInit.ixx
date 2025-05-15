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

#define FUNCPLACE(func) #func, func

export int HandleDatabaseServerInit(DNServer::Ptr dnServer)
{
	DatabaseMessageHandle::RegMsgHandle();

	if (DNClientProxy::Ptr proxy = dnServer->GetComponent<DNClientProxy>(EMComponentType::DNClientProxy))
	{
		DNClientProxy::WPtr clientProxy = proxy->GetSelfW<DNClientProxy>();
		
		proxy->onConnection = [clientProxy](const hv::SocketChannelPtr& channel)
			{
				DNClientProxy::Ptr proxy = clientProxy.lock();

				if(!proxy){ return ;}

				const std::string& peeraddr = channel->peeraddr();

				if (channel->isConnected())
				{
					proxy->GetLogger()->Record(EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());
					proxy->SetRegistEvent(&DatabaseMessage::Evt_ReqRegistSrv);
					TickMainSpaceDll(proxy, FUNCPLACE(&DNClientProxy::InitConnectedChannel),  channel);
				}
				else
				{
					proxy->GetLogger()->Record(EL10nCode_SrvConnOff, peeraddr, channel->fd(), channel->id());

					std::string origin = std::format("{}:{}", proxy->GetCtlIp(), proxy->GetCtlPort());
					if (proxy->EMRegistState() == EMRegistState::Registed || peeraddr != origin)
					{
						proxy->EMRegistState() = EMRegistState::None;

						if (proxy->isConnected())
						{
							proxy->Timer()->setTimeout(200, [=](uint64_t timerID)
								{
									proxy->GetLogger()->Record(ELogLevel_Debug, "orgin not match peeraddr {} reclient ~", origin);
									TickMainSpaceDll(proxy, FUNCPLACE(&DNClientProxy::RedirectClient),  serverProxy->GetCtlPort(), serverProxy->GetCtlIp());
								});
						}
					}

					proxy->RegistType() = 0;
				}

				if (proxy->isReconnect())
				{
				}
			};

		proxy->onMessage = [clientProxy](const hv::SocketChannelPtr& channel, hv::Buffer* buf)
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
					DatabaseMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Ret)
				{
					DatabaseMessageHandle::MsgRetHandle(channel, packet.msgHashId, msgData);
				}
				else if (packet.dealType == EMMsgDeal::Res)
				{
					if (DNTask<Message*>* task = proxy->GetMsg(packet.msgId)) // client sock request
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

export int HandleDatabaseServerShutdown(DNServer::Ptr server)
{

	if (DNClientProxy::Ptr proxy = dnServer->GetComponent<DNClientProxy>(EMComponentType::DNClientProxy))
	{
		proxy->onConnection = nullptr;
		proxy->onMessage = nullptr;
		proxy->SetRegistEvent(nullptr);

		proxy->MsgMapClear();
	}

	return true;
}
