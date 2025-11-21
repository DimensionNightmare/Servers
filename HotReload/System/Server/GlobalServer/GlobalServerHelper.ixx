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
		ServerEntityManagerHelper::CVPtr entityMan = GetServerEntityManager();

		auto selects = entityMan->GetEntitysByType(EMServerType::GateServer)
			| std::views::filter([](const auto& gate)
				{
					return gate->HasFlag(EMServerEntityFlag::Locked) == false;
				})
			| std::ranges::views::transform([](const auto& gate)
				{
					return gate->GetSelf<ServerEntityHelper>();
				});

		if (selects.empty())
		{
			return;
		}

		auto dbs = entityMan->GetEntitysByType(EMServerType::DatabaseServer)
			| std::views::transform([](const auto& param){
				return param->GetSelf<ServerEntityHelper>();
			});

		auto logics = entityMan->GetEntitysByType(EMServerType::LogicServer)
			| std::views::transform([](const auto& param){
				return param->GetSelf<ServerEntityHelper>();
			});

		// alloc gate
		GMsg::COM_RetChangeCtlSrv request;
		ServerProxyHelper::CVPtr proxyHelper = GetServerProxy();

		auto registControl = [&](ServerEntityHelper::CVPtr beEntityHelper, ServerEntityHelper::CVPtr entityHelper) ->bool
		{
			SocketChannel::CVPtr channel = entityHelper->GetChannel();
			if(!channel)
			{
				return false;
			}
			entityHelper->SetLinkNode(beEntityHelper);

			channel->deleteContextPtr();

			// sendData
			request.set_serverip(beEntityHelper->GetServerIp());
			request.set_serverport(beEntityHelper->GetServerPort());

			// timer destory
			size_t timerId = entityMan->CheckEntityCloseTimer(entityHelper->ID());
			entityHelper->SetTimerId(timerId);

			proxyHelper->AddMsg(EMMsgDeal::Ret, &request, channel).Resume();

			entityHelper->SetChannel(nullptr);

			return true;
		};

	
		for (const auto& gate : selects)
		{
			
			std::list<ServerEntity::Ptr>& gatesDb = gate->GetMapLinkNode(EMServerType::DatabaseServer);
			std::list<ServerEntity::Ptr>& gatesLogic = gate->GetMapLinkNode(EMServerType::LogicServer);
			if (!dbs.empty() && gatesDb.size() < 1)
			{
				ServerEntityHelper::CVPtr dbHelper = dbs.front();
				// dbs.pop_front();
				if(registControl(gate, dbHelper))
				{
					entityMan->UnMountEntity(dbHelper);
					gatesDb.emplace_back(dbHelper);
				}
			}

			if (!logics.empty() && gatesLogic.size() < 1)
			{
				ServerEntityHelper::CVPtr logicHelper = logics.front();
				// logics.pop_front();
				if(registControl(gate, logicHelper))
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
			proxy->onConnection = [this](SocketChannel::CVPtr channel)
				{
					ServerProxyHelper::CVPtr proxyHelper = GetServerProxy();

					if(!proxyHelper){ return ;}

					const std::string& peeraddr = channel->peeraddr();
					if (channel->isConnected())
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_CliConnOn, peeraddr, channel->fd(), channel->id());

						channel->SetWorld(GetWorld());

						proxyHelper->InitConnectedChannel(channel);
					}
					else
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());

						if (ServerEntity::CVPtr entity = channel->getContextPtr<ServerEntity>())
						{
							ServerEntityManagerHelper::CVPtr entityMan = GetServerEntityManager();
							entityMan->RemoveEntity(entity->ID());
							channel->deleteContextPtr();
						}
					}
				};

			proxy->onMessage = [this](SocketChannel::CVPtr channel, hv::Buffer* buf)
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

		if (ClientProxyHelper::Ptr proxy = GetClientProxy())
		{
			proxy->onConnection = [this](SocketChannel::CVPtr channel)
				{
					ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}

					const std::string& peeraddr = channel->peeraddr();

					if (channel->isConnected())
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());

						channel->SetWorld(GetWorld());

						GetWorld()->RemoveEvent(EMEventType::ClientProxyRegist);
						GetWorld()->AddEvent<&GlobalServerHelper::HandleClientRegist>(EMEventType::ClientProxyRegist, GetSelf<GlobalServerHelper>());

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

			proxy->onMessage = [this](SocketChannel::CVPtr channel, hv::Buffer* buf)
				{
					ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

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
