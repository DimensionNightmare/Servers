export module GateServerHelper;

import ThirdParty.PbGen;
import Server;
import ClientProxyHelper;
import ServerProxyHelper;
import ServerEntityManagerHelper;
import ProxyEntityManagerHelper;
import FuncHelper;
import MessagePack;
import ECSW;
import ServerEntityHelper;
import ProxyEntityHelper;
import FuncUtils;
import Logger;
import GateServerMessage;
import StrUtils;

export class GateServerHelper : public Helper<GateServerHelper, Server>
{

private:

	GateServerHelper() = delete;
	~GateServerHelper() = default;

public:

	ClientProxyHelper::Ptr GetClientProxy()
	{ 
		return GetComponent<ClientProxyHelper>(EMComponentType::ClientProxy);
	}

	ServerProxyHelper::Ptr GetServerProxy() 
	{
		return GetComponent<ServerProxyHelper>(EMComponentType::ServerProxy);
	}

	ServerEntityManagerHelper::Ptr GetServerEntityManager() 
	{
		return GetComponent<ServerEntityManagerHelper>(EMComponentType::ServerEntityManager);
	}

	ProxyEntityManagerHelper::Ptr GetProxyEntityManager() 
	{
		return GetComponent<ProxyEntityManagerHelper>(EMComponentType::ProxyEntityManager);
	}

	/// @brief send close to change socket
	void ServerEntityCloseEvent(Entity::Ptr entity)
	{
		// up to Global
		std::string binData;
		GMsg::g2G_RetRegistSrv request;
		request.set_serverid(entity->ID());
		request.set_isregist(false);
		request.SerializeToString(&binData);
		MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, GetClientProxy()->GetChannel());

		GetServerEntityManager()->RemoveEntity(entity->ID());
	}

	void ProxyEntityCloseEvent(Entity::Ptr entity)
	{
		ProxyEntityManagerHelper::Ptr entityMan = GetProxyEntityManager();
		size_t entityId = entity->ID();

		ServerEntityHelper::Ptr serverEntity = nullptr;
		if (size_t serverId = entity->GetSelf<ProxyEntityHelper>()->RecordServerId())
		{
			serverEntity = GetServerEntityManager()->GetEntity(serverId);
		}

		if (serverEntity)
		{
			std::string binData;
			GMsg::g2L_RetProxyOffline request;
			request.set_entityid(entityId);
			request.SerializeToString(&binData);
			MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, serverEntity->GetChannel());
		}

		entityMan->RemoveEntity(entityId);
	}

	int HandleServerInit()
	{

		if (ServerProxyHelper::Ptr proxy = GetServerProxy())
		{
			proxy->onConnection = [this](SocketChannel::Ptr channel)
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
						if (Entity::Ptr entity = channel->getContextPtr<Entity>())
						{
							switch (entity->GetEntityType())
							{
								case EMEntityType::Server:
									ServerEntityCloseEvent(entity);
									break;
								case EMEntityType::Proxy:
									ProxyEntityCloseEvent(entity);
									break;
								default:
									break;

							}

							channel->deleteContextPtr();
						}
					}
				};

			proxy->onMessage = [this](SocketChannel::Ptr channel, hv::Buffer* buf)
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

		if (ClientProxyHelper::Ptr proxy = GetClientProxy())
		{
			proxy->onConnection = [this](SocketChannel::Ptr channel)
				{
					ClientProxyHelper::Ptr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}

					const std::string& peeraddr = channel->peeraddr();

					if (channel->isConnected())
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());

						channel->SetWorld(GetWorldW());
						
						proxyHelper->SetRegistEvent(ServerMessage::GetMessageHandle()->pClientRegistFunc);
						proxyHelper->InitConnectedChannel(channel);
					}
					else
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_SrvConnOff, peeraddr, channel->fd(), channel->id());
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

			proxy->onMessage = [this](SocketChannel::Ptr channel, hv::Buffer* buf)
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

					if (packet->dealType == EMMsgDeal::Req)
					{
						ServerMessage::GetMessageHandle()->MsgHandle(channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Redir)
					{
						ServerMessage::GetMessageHandle()->MsgRedirectHandle(channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Res)
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
		if (ServerProxyHelper::Ptr proxy = GetServerProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;

			proxy->MsgMapClear();
		}

		if (ClientProxyHelper::Ptr proxy = GetClientProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;
			proxy->SetRegistEvent(nullptr);

			proxy->MsgMapClear();
		}

		return true;
	}

};
