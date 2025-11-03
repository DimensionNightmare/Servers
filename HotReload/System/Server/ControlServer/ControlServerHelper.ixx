export module ControlServerHelper;

import Server;
import ServerProxyHelper;
import ServerEntityManagerHelper;
import MessagePack;
import FuncUtils;
import Logger;
import ControlServerMessage;
import StrUtils;

export class ControlServerHelper : public Helper<ControlServerHelper, Server>
{
private:

	ControlServerHelper() = delete;
	~ControlServerHelper() = default;

public:

	ServerProxyHelper::Ptr GetServerProxy() 
	{
		return GetComponent<ServerProxyHelper>(EMComponentType::ServerProxy);
	}

	ServerEntityManagerHelper::Ptr GetServerEntityManager() 
	{
		return GetComponent<ServerEntityManagerHelper>(EMComponentType::ServerEntityManager);
	}

	
	void HandleServerInit()
	{

		if (ServerProxyHelper::Ptr proxy = GetServerProxy())
		{
			proxy->onConnection = [this](const SocketChannel::Ptr& channel)
				{
					ServerProxyHelper::Ptr proxyHelper = GetServerProxy();

					if(!proxyHelper){ return ;}

					const std::string& peeraddr = channel->peeraddr();
					if (channel->isConnected())
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_CliConnOn, peeraddr, channel->fd(), channel->id());

						channel->SetWorld(GetWorldW());
						
						proxyHelper->InitConnectedChannel(channel);
					}
					else
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());

						// not used
						if (ServerEntity::Ptr entity = channel->getContextPtr<ServerEntity>())
						{
							ServerEntityManagerHelper::Ptr entityMan = GetServerEntityManager();
							entityMan->RemoveEntity(entity->ID());
							channel->deleteContextPtr();
						}

					}
				};

			proxy->onMessage = [this](const SocketChannel::Ptr& channel, hv::Buffer* buf)
				{
					ServerProxyHelper::Ptr proxyHelper = GetServerProxy();

					if(!proxyHelper){ return ;}
					
					MessagePacket* packet = MessagePacket::From(buf->data());

					LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "s {} Recv type={} With Mid:{}", channel->peeraddr(), EnumName(packet->dealType), packet->msgId);

					if(packet->pkgLenth > 2 * 1024)
					{
						LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Recv byte len limit={}", packet->pkgLenth);
						return;
					}
					
					std::string msgData(packet->MsgBegin(), packet->pkgLenth);
					
					if (packet->dealType == EMMsgDeal::Req)
					{
						ServerMessage::GetMessageHandle()->MsgHandle(channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Ret)
					{
						ServerMessage::GetMessageHandle()->MsgRetHandle(channel, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Redir)
					{
						ServerMessage::GetMessageHandle()->MsgRedirectHandle(channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Res)
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
		if (ServerProxyHelper::Ptr proxy = GetServerProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;

			proxy->ClearMsgMap();
		}

		return;
	}

};
