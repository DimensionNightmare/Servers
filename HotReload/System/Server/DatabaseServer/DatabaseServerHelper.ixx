export module DatabaseServerHelper;

import ThirdParty.PbGen;
import ClientProxyHelper;
import ServerEntityManagerHelper;
import DbUtils;
import RdbProxyHelper;
import StrUtils;
import Server;
import MessagePack;
import FuncUtils;
import Logger;
import DatabaseServerMessage;

export class DatabaseServerHelper : public Helper<DatabaseServerHelper, Server>
{

private:

	DatabaseServerHelper() = delete;
	~DatabaseServerHelper() = default;

public:

	ClientProxyHelper::Ptr GetClientProxy()
	{ 
		return GetComponent<ClientProxyHelper>(EMComponentType::ClientProxy);
	}

	ServerEntityManagerHelper::Ptr GetServerEntityManager() 
	{
		return GetComponent<ServerEntityManagerHelper>(EMComponentType::ServerEntityManager);
	}

	RdbProxyHelper::Ptr GetRdbProxy()
	{ 
		return GetComponent<RdbProxyHelper>(EMComponentType::RdbProxy);
	}

	bool CheckDatabase()
	{
		if(RdbProxyHelper::Ptr proxy = GetRdbProxy())
		{
			try
			{
				#define REMOVE_CV(TYPE) static_cast<Message*>(const_cast<TYPE*>(TYPE::internal_default_instance()))

				std::unordered_map<EMSqlDbNameEnum, std::vector<Message*> > registTable = {
					{
						EMSqlDbNameEnum::Account,
						{
							REMOVE_CV(GDb::Account),
						}
					},
					{
						EMSqlDbNameEnum::Nightmare,
						{
							REMOVE_CV(GDb::Player),
						}
					},
				};

				#undef REMOVE_CV

				GDb::SingleTon kv;
				std::string schemaMd5;

				for (const auto& [dbNameEnum, dbEntitys] : registTable)
				{
					if (auto transaction = proxy->GetTransaction(dbNameEnum, false))
					{
						DbSqlHelper<GDb::SingleTon> singleTon(transaction.get(), GetWorld());
						singleTon.InitEntity(kv);

						if (!singleTon.IsExist())
						{
							LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Create Table:SingleTon");
							singleTon.CreateTable().Commit();
						}

						for (const auto& dbEntity : dbEntitys)
						{
							DbSqlHelper helper(transaction.get(), GetWorld(), dbEntity);

							const std::string& tableName = helper.GetName();
							kv.set_key(std::format("{}_Schema", tableName));
							schemaMd5 = helper.GetTableSchemaMd5();

							bool hasCreated = false;

							if (!helper.IsExist())
							{

								LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Create Table:{}", tableName);
								helper.CreateTable().Commit();

								hasCreated = true;
							}

							// #define DBSelectCond(obj, name, cond, splicing) .SelectCond(#name, cond, splicing, [&obj]() { return obj.name(); })
							// #define DBUpdate(obj, name) .Update(obj, #name, [&obj]() { return obj.name(); })
							// #define DBUpdateCond(obj, name, cond, splicing) .UpdateCond(#name, cond, splicing, [&obj]() { return obj.name(); })
							// #define DBDeleteCond(obj, name, cond, splicing) .DeleteCond(#name, cond, splicing, [&obj]() { return obj.name(); })

							// // key method
							// #define DBUpdateByKey(obj, name) .UpdateByKey(#name, [&obj]() { return obj.name(); })
						
							singleTon
								.SelectByKey<GDb::SingleTon::kKeyFieldNumber>()
								.Commit();

							if (singleTon.IsSuccess())
							{
								if(singleTon.Result().size() > 0)
								{
									kv = *singleTon.Result()[0];
									if (schemaMd5 == kv.value())
									{
										continue;
									}
								}
								else
								{
									if(hasCreated)
									{
										kv.set_value(schemaMd5);
										singleTon.Insert().Commit();
										continue;
									}
								}
								
							}

							LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "not match md5:\n{}\n{}", schemaMd5, kv.value());
							helper.UpdateTable().Commit();


							kv.set_value(schemaMd5);
							singleTon.UpdateByKey<GDb::SingleTon::kKeyFieldNumber>().Commit();
							
						}

						transaction->commit();
					}
				}
			}
			catch (const std::exception& e)
			{
				LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "{}", e.what());
				return false;
			}

			return true;
		}

		return false;
	}
	
	void HandleServerInit()
	{

		if (ClientProxyHelper::Ptr proxy = GetClientProxy())
		{
			proxy->onConnection = [this](const SocketChannel::Ptr& channel)
				{
					ClientProxyHelper::Ptr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}

					const std::string& peeraddr = channel->peeraddr();

					if (channel->isConnected())
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());

						channel->SetWorld(GetWorldW());
						
						GetWorld()->RemoveEvent(EMEventType::ClientProxyRegist);
						GetWorld()->AddEvent<&DatabaseServerHelper::HandleClientRegist>(EMEventType::ClientProxyRegist, GetSelf<DatabaseServerHelper>());
						proxyHelper->InitConnectedChannel(channel);
					}
					else
					{
						LoggerPrint::Log(GetWorld(), EL10nCode_SrvConnOff, peeraddr, channel->fd(), channel->id());

						std::string originIp;
						if(std::string* param = GetWorld()->GetParam("ctlIp"))
						{
							originIp = *param;
						}

						std::string originPort;
						if(std::string* param = GetWorld()->GetParam("ctlPort"))
						{
							originPort = *param;
						}

						std::string origin = std::format("{}:{}", originIp, originPort);

						if (proxyHelper->GetRegistState() == EMRegistState::Registed || peeraddr != origin)
						{
							proxyHelper->SetRegistState(EMRegistState::None);

							if (proxyHelper->isConnected())
							{
								LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "orgin not match peeraddr {} reclient ~", origin);

								proxyHelper->GetTimer()->SetTimeout(200, [this, originIp, originPort](size_t timerID)
									{
										ClientProxyHelper::Ptr proxyHelper = GetClientProxy();
										if(!proxyHelper){ return ;}
										proxyHelper->RedirectClient(std::stoi(originPort), originIp);
									});
							}
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

					if (packet->dealType == EMMsgDeal::Req)
					{
						ServerMessage::GetMessageHandle()->MsgHandle(channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Ret)
					{
						ServerMessage::GetMessageHandle()->MsgRetHandle(channel, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Res)
					{
						if (MsgTask* task = proxyHelper->GetMsg(packet->msgId)) // client sock request
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

		GetWorld()->AddEvent<&DatabaseServerHelper::CheckDatabase>(EMEventType::InitedRdbConnection, GetSelf<DatabaseServerHelper>());
	}

	void HandleServerShutdown()
	{
		if (ClientProxyHelper::Ptr proxy = GetClientProxy())
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
