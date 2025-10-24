export module AuthServerHelper;

import ThirdParty.PbGen;
import ClientProxyHelper;
import WebProxyHelper;
import DbUtils;
import RdbProxyHelper;
import StrUtils;
import Server;
import MessagePack;
import ECSW;
import MessageRegister;
import FuncUtils;
import Logger;

export class AuthServerHelper : public Helper<AuthServerHelper, Server>
{

private:

	AuthServerHelper() = delete;
	~AuthServerHelper() = default;

public:

	ClientProxyHelper::Ptr GetClientProxy()
	{ 
		ClientProxyHelper::Ptr proxy = GetComponent<ClientProxyHelper>(EMComponentType::ClientProxy);
		if(!proxy || proxy->IsDisposed())
		{
			return nullptr;
		}
		return proxy;
	}

	WebProxyHelper::Ptr GetWebProxy()
	{ 
		WebProxyHelper::Ptr proxy = GetComponent<WebProxyHelper>(EMComponentType::WebProxy);
		if(!proxy || proxy->IsDisposed())
		{
			return nullptr;
		}
		return proxy;
	}

	RdbProxyHelper::Ptr GetRdbProxy()
	{ 
		RdbProxyHelper::Ptr proxy = GetComponent<RdbProxyHelper>(EMComponentType::RdbProxy);
		if(!proxy || proxy->IsDisposed())
		{
			return nullptr;
		}
		return proxy;
	}

	int HandleServerInit(MessageRegister* msgHandle)
	{
		
		msgHandle->RegApiHandle(GetSelf<Server>());

		if (ClientProxyHelper::Ptr proxy = GetClientProxy())
		{
			proxy->onConnection = [this,msgHandle](SocketChannel::CVPtr channel)
				{
					ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}

					const std::string& peeraddr = channel->peeraddr();

					if (channel->isConnected())
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_CliConnOn, peeraddr, channel->fd(), channel->id());

						channel->SetWorld(GetWorldW());

						proxyHelper->SetRegistEvent(msgHandle->GetClientRegistFunc());
						proxyHelper->InitConnectedChannel(channel);
					}
					else
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());

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

			proxy->onMessage = [this](SocketChannel::CVPtr channel, hv::Buffer* buf)
				{
					ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}
					
					MessagePacket* packet = MessagePacket::From(buf->data());

					LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "c {} Recv type={} With Mid:{}", channel->peeraddr(), static_cast<int>(packet->dealType), packet->msgId);

					if(packet->pkgLenth > 2 * 1024)
					{
						LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Recv byte len limit={}", packet->pkgLenth);
						return;
					}

					std::string msgData(packet->MsgBegin(), packet->pkgLenth);

					if (packet->dealType == EMMsgDeal::Res)
					{
						if (Task<Message*>* task = proxyHelper->GetMsg(packet->msgId)) //client sock request
						{
							proxyHelper->DelMsg(packet->msgId);
							task->Resume();

							if (Message* message = task->GetResult())
							{
								if (!message->ParseFromString(msgData))
								{
									task->SetFlag(EMTaskFlag::PaserError);
								}
							}

							task->CallResume();
						}
						else
						{
							LoggerPrint::Log(GetWorld(), EL10nCode_MsgFind);
						}
					}
					else
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_MsgDealType);
					}
				};

		}
		

		return true;
	}

	int HandleServerShutdown()
	{
		if (ClientProxyHelper::CVPtr proxy = GetClientProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;
			proxy->SetRegistEvent(nullptr);

			// web use clientMsg
			proxy->MsgMapClear();
		}

		if (WebProxyHelper::CVPtr proxy = GetWebProxy())
		{
			if(proxy->service)
			{
				std::function<int(const hv::HttpContextPtr&)> funcReplace = nullptr;
				proxy->service->preprocessor = funcReplace;
				proxy->service->pathHandlers.clear();
			}
		}

		return true;
	}

};
