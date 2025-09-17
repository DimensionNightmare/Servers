export module DatabaseServerHelper;

import ThirdParty.PbGen;
import ClientProxyHelper;
import ServerEntityManagerHelper;
import DbUtils;
import RdbProxyHelper;
import StrUtils;
import Server;
import MessagePack;
import ECSW;
import MessageRegister;
import FuncUtils;

export class DatabaseServerHelper : public Helper<DatabaseServerHelper, Server>
{

private:

	DatabaseServerHelper() = delete;
	~DatabaseServerHelper() = default;

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

	ServerEntityManagerHelper::Ptr GetServerEntityManager() 
	{
		ServerEntityManagerHelper::Ptr proxy = GetComponent<ServerEntityManagerHelper>(EMComponentType::ServerEntityManager);
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

	bool CheckDatabase()
	{
		if(RdbProxyHelper::CVPtr proxy = GetRdbProxy())
		{
			try
			{
				World::CVPtr world = GetWorld();

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

				for (auto& [dbNameEnum, dbEntitys] : registTable)
				{
					if (auto connection = proxy->GetConnection(dbNameEnum))
					{
						pqxx::work txn(*connection);
						DbSqlHelper<GDb::SingleTon> singleTon(&txn, GetLogger());
						singleTon.InitEntity(kv);

						if (!singleTon.IsExist())
						{
							GetLogger()->Record(ELogLevel_Debug, "Create Table:SingleTon");
							singleTon.CreateTable().Commit();
						}

						for (Message* dbEntity : dbEntitys)
						{
							DbSqlHelper helper(&txn, GetLogger(), dbEntity);

							const std::string& tableName = helper.GetName();
							kv.set_key(std::format("{}_Schema", tableName));
							schemaMd5 = helper.GetTableSchemaMd5();

							bool hasCreated = false;

							if (!helper.IsExist())
							{

								GetLogger()->Record(ELogLevel_Debug, "Create Table:{}", tableName);
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
							
							GetLogger()->Record(ELogLevel_Debug, "not match md5:\n{}\n{}", schemaMd5, kv.value());
							helper.UpdateTable().Commit();


							kv.set_value(schemaMd5);
							singleTon.UpdateByKey<GDb::SingleTon::kKeyFieldNumber>().Commit();
							
						}

						txn.commit();
					}
				}
			}
			catch (const std::exception& e)
			{
				GetLogger()->Record(ELogLevel_Debug, "{}", e.what());
				return false;
			}

			return true;
		}

		return false;
	}
	
	int HandleServerInit(MessageRegister* msgHandle)
	{
		msgHandle->RegMsgHandle();

		if (ClientProxy::CVPtr proxy = GetComponent<ClientProxy>(EMComponentType::ClientProxy))
		{
			proxy->onConnection = [this,msgHandle](SocketChannel::CVPtr channel)
				{
					ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}

					const std::string& peeraddr = channel->peeraddr();

					if (channel->isConnected())
					{
						GetLogger()->Record(EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());

						channel->SetWorld(GetWorldW());
						
						proxyHelper->SetRegistEvent(msgHandle->GetClientRegistFunc());
						proxyHelper->InitConnectedChannel(channel);
					}
					else
					{
						GetLogger()->Record(EL10nCode_SrvConnOff, peeraddr, channel->fd(), channel->id());

						std::string originIp;
						if(std::string* param = GetWorld()->LaunchParam("ctlIp"))
						{
							originIp = *param;
						}

						std::string originPort;
						if(std::string* param = GetWorld()->LaunchParam("ctlPort"))
						{
							originPort = *param;
						}

						std::string origin = std::format("{}:{}", originIp, originPort);

						if (proxyHelper->GetRegistState() == EMRegistState::Registed || peeraddr != origin)
						{
							proxyHelper->SetRegistState(EMRegistState::None);

							if (proxyHelper->isConnected())
							{
								GetLogger()->Record(ELogLevel_Debug, "orgin not match peeraddr {} reclient ~", origin);

								proxyHelper->GetTimer()->SetTimeout(200, [this, originIp, originPort](uint64_t timerID)
									{
										ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();
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

			proxy->onMessage = [this,msgHandle](SocketChannel::CVPtr channel, hv::Buffer* buf)
				{
					ClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}

					MessagePacket* packet = MessagePacket::From(buf->data());

					GetLogger()->Record(ELogLevel_Debug, "c {} Recv type={} With Mid:{}", channel->peeraddr(), static_cast<int>(packet->dealType), packet->msgId);

					if(packet->pkgLenth > 2 * 1024)
					{
						GetLogger()->Record(ELogLevel_Debug, "Recv byte len limit={}", packet->pkgLenth);
						return;
					}
					
					std::string msgData(packet->MsgBegin(), packet->pkgLenth);

					if (packet->dealType == EMMsgDeal::Req)
					{
						msgHandle->MsgHandle(channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Ret)
					{
						msgHandle->MsgRetHandle(channel, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Res)
					{
						if (Task<Message*>* task = proxyHelper->GetMsg(packet->msgId)) // client sock request
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
							GetLogger()->Record(EL10nCode_MsgFind);
						}
					}
					else
					{
						GetLogger()->Record(EL10nCode_MsgDealType);
					}
				};

		}

		return CheckDatabase();
	}

	int HandleServerShutdown()
	{
		if (ClientProxyHelper::CVPtr proxy = GetClientProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;
			proxy->SetRegistEvent(nullptr);

			proxy->MsgMapClear();
		}

		return true;
	}

};
