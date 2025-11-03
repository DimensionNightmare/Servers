export module AuthServerHelper;

import ClientProxyHelper;
import WebProxyHelper;
import DbUtils;
import RdbProxyHelper;
import StrUtils;
import Server;
import MessagePack;
import FuncUtils;
import Logger;
import AuthServerMessage;

export class AuthServerHelper : public Helper<AuthServerHelper, Server>
{

private:

	AuthServerHelper() = delete;
	~AuthServerHelper() = default;

public:

	ClientProxyHelper::Ptr GetClientProxy()
	{ 
		return GetComponent<ClientProxyHelper>(EMComponentType::ClientProxy);
	}

	WebProxyHelper::Ptr GetWebProxy()
	{ 
		return GetComponent<WebProxyHelper>(EMComponentType::WebProxy);
	}

	RdbProxyHelper::Ptr GetRdbProxy()
	{ 
		return GetComponent<RdbProxyHelper>(EMComponentType::RdbProxy);
	}

	void HandleServerInit()
	{
		ServerMessage::GetMessageHandle()->pApiRegistFunc(GetSelf<Server>());
		
		if (ClientProxyHelper::Ptr proxy = GetClientProxy())
		{
			proxy->onConnection = [this](const SocketChannel::Ptr& channel)
				{
					ClientProxyHelper::Ptr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}

					const std::string& peeraddr = channel->peeraddr();

					if (channel->isConnected())
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_CliConnOn, peeraddr, channel->fd(), channel->id());

						channel->SetWorld(GetWorldW());

						GetWorld()->AddEvent<&AuthServerHelper::HandleClientRegist>(EMEventType::ClientProxyRegist, GetSelf<AuthServerHelper>());
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

			proxy->onMessage = [this](const SocketChannel::Ptr& channel, hv::Buffer* buf)
				{
					ClientProxyHelper::Ptr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}
					
					MessagePacket* packet = MessagePacket::From(buf->data());

					LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "c {} Recv type={} With Mid:{}", channel->peeraddr(), EnumName(packet->dealType), packet->msgId);

					if(packet->pkgLenth > 2 * 1024)
					{
						LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Recv byte len limit={}", packet->pkgLenth);
						return;
					}

					std::string msgData(packet->MsgBegin(), packet->pkgLenth);

					if (packet->dealType == EMMsgDeal::Res)
					{
						if (MsgTask* task = proxyHelper->GetMsg(packet->msgId)) //client sock request
						{
							proxyHelper->DelMsg(packet->msgId);

							if (Message* message = task->GetMessage())
							{
								if (!message->ParseFromString(msgData))
								{
									task->SetFlag(EMTaskFlag::PaserError);
								}
							}

							task->Resume();
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
		

		return;
	}

	void HandleServerShutdown()
	{
		if (ClientProxyHelper::Ptr proxy = GetClientProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;

			// web use clientMsg
			proxy->ClearMsgMap();
		}

		if (WebProxyHelper::Ptr proxy = GetWebProxy())
		{
			if(proxy->service)
			{
				std::function<int(const hv::HttpContextPtr&)> funcReplace = nullptr;
				proxy->service->preprocessor = funcReplace;
				proxy->service->pathHandlers.clear();
			}
		}

		return;
	}

	void HandleClientRegist()
	{
		ServerMessage::GetMessageHandle()->pClientRegistFunc(GetSelf<Server>());
	}
};
