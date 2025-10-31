export module GlobalServerHelper;

import ThirdParty.PbGen;
import ClientProxyHelper;
import ServerProxyHelper;
import ServerEntityManagerHelper;
import FuncHelper;
import Server;
import MessagePack;
import FuncUtils;
import Logger;
import GlobalServerMessage;
import StrUtils;

export class GlobalServerHelper : public Helper<GlobalServerHelper, Server>
{

private:

	GlobalServerHelper() = delete;
	~GlobalServerHelper() = default;

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

	void UpdateServerGroup()
	{
		ServerEntityManagerHelper::Ptr entityMan = GetServerEntityManager();

		std::list<ServerEntity::Ptr>& gates = entityMan->GetEntitysByType(EMServerType::GateServer);
		if (gates.empty())
		{
			return;
		}

		std::list<ServerEntity::Ptr>& dbs = entityMan->GetEntitysByType(EMServerType::DatabaseServer);
		std::list<ServerEntity::Ptr>& logics = entityMan->GetEntitysByType(EMServerType::LogicServer);

		// alloc gate
		GMsg::COM_RetChangeCtlSrv request;
		std::string binData;

		auto registControl = [&](ServerEntityHelper::Ptr beEntityHelper, ServerEntityHelper::Ptr entityHelper) ->bool
		{
			SocketChannel::Ptr channel = entityHelper->GetChannel();
			if(!channel)
			{
				return false;
			}
			entityHelper->SetLinkNode(beEntityHelper);

			channel->deleteContextPtr();

			// sendData
			request.set_serverip(beEntityHelper->GetServerIp());
			request.set_serverport(beEntityHelper->GetServerPort());

			request.SerializeToString(&binData);
			// timer destory
			size_t timerId = entityMan->CheckEntityCloseTimer(entityHelper->ID());
			entityHelper->SetTimerId(timerId);

			MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, channel);
			entityHelper->SetChannel(nullptr);

			return true;
		};

		for (ServerEntity::Ptr& gate : gates)
		{
			if (gate->HasFlag(EMServerEntityFlag::Locked))
			{
				continue;
			}

			ServerEntityHelper::Ptr gateHelper = gate->GetSelf<ServerEntityHelper>();

			std::list<ServerEntity::Ptr>& gatesDb = gate->GetMapLinkNode(EMServerType::DatabaseServer);
			std::list<ServerEntity::Ptr>& gatesLogic = gate->GetMapLinkNode(EMServerType::LogicServer);
			if (!dbs.empty() && gatesDb.size() < 1)
			{
				ServerEntityHelper::Ptr dbHelper = dbs.front()->GetSelf<ServerEntityHelper>();
				// dbs.pop_front();
				if(registControl(gateHelper, dbHelper))
				{
					entityMan->UnMountEntity(dbHelper);
					gatesDb.emplace_back(dbHelper);
				}
			}

			if (!logics.empty() && gatesLogic.size() < 1)
			{
				ServerEntityHelper::Ptr logicHelper = logics.front()->GetSelf<ServerEntityHelper>();
				// logics.pop_front();
				if(registControl(gateHelper, logicHelper))
				{
					entityMan->UnMountEntity(logicHelper);
					gatesLogic.emplace_back(logicHelper);
				}
			}

			if (gatesDb.size() && gatesLogic.size())
			{
				// UnMountEntity(gate->GetServerType(), it);
				gate->SetFlag(EMServerEntityFlag::Locked);
				LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Gate:{} locked!", gate->ID());
			}

		}
	}

	void HandleServerInit()
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

						if (ServerEntity::Ptr entity = channel->getContextPtr<ServerEntity>())
						{
							ServerEntityManagerHelper::Ptr entityMan = GetServerEntityManager();
							entityMan->RemoveEntity(entity->ID());
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

		return;

	}

	void HandleServerShutdown()
	{
		
		if (ServerProxyHelper::Ptr serverSock = GetServerProxy())
		{
			serverSock->onConnection = nullptr;
			serverSock->onMessage = nullptr;

			serverSock->ClearMsgMap();
		}

		if (ClientProxyHelper::Ptr clientSock = GetClientProxy())
		{
			clientSock->onConnection = nullptr;
			clientSock->onMessage = nullptr;
			clientSock->SetRegistEvent(nullptr);

			clientSock->ClearMsgMap();
		}

		return;
	}

};
