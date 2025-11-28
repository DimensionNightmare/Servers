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
import MdbProxyHelper;

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

	MdbProxyHelper::Ptr GetMdbProxy()
	{ 
		return GetComponent<MdbProxyHelper>(EMComponentType::MdbProxy);
	}

	void HandleServerInit()
	{
		ServerMessage::GetMessageHandle()->pApiRegistFunc(GetSelf<Server>());
		
		if (ClientProxyHelper::CVPtr proxy = GetClientProxy())
		{
			proxy->onConnection = [this](SocketChannel::CVPtr channel)
				{
					World::CVPtr world = GetWorld();
					
					ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();
					
					const std::string& peeraddr = channel->peeraddr();

					if (channel->isConnected())
					{
						LoggerPrint::Log(world, EL10nCode_CliConnOn, peeraddr, channel->fd(), channel->id());
						proxyHelper->InitConnectedChannel(channel);
						
						world->RemoveEvent(EMEventType::ClientProxyRegist);
						world->AddEvent<&AuthServerHelper::HandleClientRegist>(EMEventType::ClientProxyRegist, GetSelf<AuthServerHelper>());
					}
					else
					{
						LoggerPrint::Log(world, EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());

						if (proxyHelper->GetRegistState() == EMRegistState::Registed)
						{
							proxyHelper->SetRegistState(EMRegistState::None);
						}

						proxyHelper->SetRegistType(0);
					}
				};

			proxy->onMessage = [this](SocketChannel::CVPtr channel, hv::Buffer* buf)
				{
					MessagePacket* packet = MessagePacket::From(buf->data());

					if(packet->pkgLenth > 2 * 1024)
					{
						LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Recv byte len limit={}", packet->pkgLenth);
						return;
					}

					std::string msgData(packet->MsgBegin(), packet->pkgLenth);

					if (packet->dealType == EMMsgDeal::Res)
					{
						ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

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
		if (ClientProxyHelper::CVPtr proxy = GetClientProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;

			// web use clientMsg
			proxy->ClearMsgMap();
		}

		if (WebProxyHelper::CVPtr proxy = GetWebProxy())
		{
			if(proxy->service)
			{
				proxy->service->preprocessor = std::function<int(const hv::HttpContextPtr&)>();
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
