export module GateServerHelper;

import ThirdParty.PbGen;
import Server;
import ClientProxyHelper;
import ServerProxyHelper;
import ServerEntityManagerHelper;
import ProxyEntityManagerHelper;
import FuncHelper;
import MessagePack;
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
	void ServerEntityCloseEvent(Entity::CVPtr entity)
	{
		// up to Global
		GMsg::g2G_RetRegistSrv request;
		request.set_serverid(entity->ID());
		request.set_isregist(false);

		ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();
		proxyHelper->AddMsg(EMMsgDeal::Ret, &request).Resume();

		GetServerEntityManager()->DisposeEntity(entity);
	}

	void ProxyEntityCloseEvent(Entity::CVPtr entity)
	{
		ProxyEntityManagerHelper::CVPtr entityMan = GetProxyEntityManager();

		ServerEntityHelper::Ptr serverEntity;
		if (size_t serverId = entity->GetSelf<ProxyEntityHelper>()->GetRecordServerId())
		{
			serverEntity = GetServerEntityManager()->GetEntity(serverId);
		}

		if (serverEntity)
		{
			GMsg::g2L_RetProxyOffline request;
			request.set_entityid(entity->ID());

			ServerProxyHelper::CVPtr proxyHelper = GetServerProxy();

			proxyHelper->AddMsg(EMMsgDeal::Ret, &request, serverEntity->GetChannel()).Resume();
		}

		entityMan->DisposeEntity(entity);
	}

	void HandleServerInit()
	{

		if (ServerProxyHelper::CVPtr proxy = GetServerProxy())
		{
			proxy->onConnection = [this](SocketChannel::CVPtr channel)
				{
					World::CVPtr world = GetWorld();
					
					ServerProxyHelper::CVPtr proxyHelper = GetServerProxy();
					
					const std::string& peeraddr = channel->peeraddr();

					if (channel->isConnected())
					{
						LoggerPrint::Log(world, EL10nCode_CliConnOn, peeraddr, channel->fd(), channel->id());
						proxyHelper->InitConnectedChannel(channel);

					}
					else
					{
						LoggerPrint::Log(world, EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());
						if (Entity::CVPtr entity = channel->GetEntity<Entity>())
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
						}
					}
				};

			proxy->onMessage = [this](SocketChannel::CVPtr channel, hv::Buffer* buf)
				{
					MessagePacket* packet = MessagePacket::From(buf->data());

					World::CVPtr world = GetWorld();

					if(packet->pkgLenth > 2 * 1024)
					{
						LoggerPrint::Log(world, ELogLevel_Debug, "Recv byte len limit={}", packet->pkgLenth);
						return;
					}

					std::string msgData(packet->MsgBegin(), packet->pkgLenth);

					if (packet->dealType == EMMsgDeal::Req)
					{
						ServerMessage::GetMessageHandle()->MsgHandle(world, channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Ret)
					{
						ServerMessage::GetMessageHandle()->MsgRetHandle(world, channel, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Redir)
					{
						ServerMessage::GetMessageHandle()->MsgRedirectHandle(world, channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Res)
					{
						ServerProxyHelper::CVPtr proxyHelper = GetServerProxy();

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
							LoggerPrint::Log(world, EL10nCode_MsgFind);
						}
					}
					else
					{
						LoggerPrint::Log(world, EL10nCode_MsgDealType);
					}
				};

		}

		if (ClientProxyHelper::CVPtr proxy = GetClientProxy())
		{
			proxy->onConnection = [this](SocketChannel::CVPtr channel)
				{
					World::CVPtr world = GetWorld();
					
					ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();
					
					const std::string& peeraddr = channel->peeraddr();

					if (channel->isConnected())
					{
						LoggerPrint::Log(world, EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());
						proxyHelper->InitConnectedChannel(channel);

						world->RemoveEvent(EMEventType::ClientProxyRegist);
						world->AddEvent<&GateServerHelper::HandleClientRegist>(EMEventType::ClientProxyRegist, GetSelf<GateServerHelper>());
					}
					else
					{
						LoggerPrint::Log(world, EL10nCode_SrvConnOff, peeraddr, channel->fd(), channel->id());
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

					World::CVPtr world = GetWorld();

					if(packet->pkgLenth > 2 * 1024)
					{
						LoggerPrint::Log(world, ELogLevel_Debug, "Recv byte len limit={}", packet->pkgLenth);
						return;
					}
					
					std::string msgData(packet->MsgBegin(), packet->pkgLenth);

					if (packet->dealType == EMMsgDeal::Req)
					{
						ServerMessage::GetMessageHandle()->MsgHandle(world, channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Redir)
					{
						ServerMessage::GetMessageHandle()->MsgRedirectHandle(world, channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Res)
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
							LoggerPrint::Log(world, EL10nCode_MsgFind);
						}
					}
					else
					{
						LoggerPrint::Log(world, EL10nCode_MsgDealType);
					}
				};

		}

		return;
	}

	void HandleServerShutdown()
	{
		if (ServerProxyHelper::CVPtr proxy = GetServerProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;

			proxy->ClearMsgMap();
		}

		if (ClientProxyHelper::CVPtr proxy = GetClientProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;

			proxy->ClearMsgMap();
		}

		return;
	}

	void HandleClientRegist()
	{
		ServerMessage::GetMessageHandle()->pClientRegistFunc(GetSelf<Server>());
	}

};
