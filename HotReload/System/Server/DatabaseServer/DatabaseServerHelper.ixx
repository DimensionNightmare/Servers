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
		if(RdbProxyHelper::CVPtr proxy = GetRdbProxy())
		{

			World::CVPtr world = GetWorld();

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
						DbSqlHelper<GDb::SingleTon> singleTon(transaction.get(), world);
						singleTon.InitEntity(kv);

						if (!singleTon.IsExist())
						{
							LoggerPrint::Log(world, ELogLevel_Debug, "Create Table:SingleTon");
							singleTon.CreateTable().Commit();
						}

						for (const auto& dbEntity : dbEntitys)
						{
							DbSqlHelper helper(transaction.get(), world, dbEntity);

							const std::string& tableName = helper.GetName();
							kv.set_key(std::format("{}_Schema", tableName));
							schemaMd5 = helper.GetTableSchemaMd5();

							bool hasCreated = false;

							if (!helper.IsExist())
							{

								LoggerPrint::Log(world, ELogLevel_Debug, "Create Table:{}", tableName);
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
								const auto& results = singleTon.GetResult();
								if(results.size() > 0)
								{
									kv = *results[0];
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

							LoggerPrint::Log(world, ELogLevel_Debug, "not match md5:\n{}\n{}", schemaMd5, kv.value());
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
				LoggerPrint::Log(world, ELogLevel_Debug, "{}", e.what());
				return false;
			}

			return true;
		}

		return false;
	}
	
	void HandleServerInit()
	{

		if (ClientProxyHelper::CVPtr proxy = GetClientProxy())
		{
			proxy->onConnection = [this](SocketChannel::CVPtr channel)
				{
					ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

					const std::string& peeraddr = channel->peeraddr();

					World::CVPtr world = GetWorld();

					if (channel->isConnected())
					{
						LoggerPrint::Log(world, EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());

						world->RemoveEvent(EMEventType::ClientProxyRegist);
						world->AddEvent<&DatabaseServerHelper::HandleClientRegist>(EMEventType::ClientProxyRegist, GetSelf<DatabaseServerHelper>());
						proxyHelper->InitConnectedChannel(channel);
					}
					else
					{
						LoggerPrint::Log(world, EL10nCode_SrvConnOff, peeraddr, channel->fd(), channel->id());

						std::string originIp;
						if(std::string* param = world->GetParam("ctlIp"))
						{
							originIp = *param;
						}

						std::string originPort;
						if(std::string* param = world->GetParam("ctlPort"))
						{
							originPort = *param;
						}

						std::string origin = std::format("{}:{}", originIp, originPort);

						if (proxyHelper->GetRegistState() == EMRegistState::Registed || peeraddr != origin)
						{
							proxyHelper->SetRegistState(EMRegistState::None);

							if (proxyHelper->isConnected())
							{
								LoggerPrint::Log(world, ELogLevel_Debug, "orgin not match peeraddr {} reclient ~", origin);

								proxyHelper->GetTimer()->SetTimeout(200, [this, originIp, originPort](size_t timerID)
									{
										ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();
										
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
					else if (packet->dealType == EMMsgDeal::Res)
					{
						ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

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
							LoggerPrint::Log(world, EL10nCode_MsgFind);
						}
					}
					else
					{
						LoggerPrint::Log(world, EL10nCode_MsgDealType);
					}
				};

		}

		GetWorld()->AddEvent<&DatabaseServerHelper::CheckDatabase>(EMEventType::InitedRdbConnection, GetSelf<DatabaseServerHelper>());
	}

	void HandleServerShutdown()
	{
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
