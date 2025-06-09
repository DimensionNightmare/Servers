module;
export module DatabaseServerHelper;

export import ThirdParty.PbGen;
import DNClientProxyHelper;
import ServerEntityManagerHelper;
import DbUtils;
import RdbProxyHelper;
import StrUtils;
import DNServer;
import DllUtils;
import MessagePack;
import ECSW;
import MessageRegister;

#define FUNCPLACE(class, func) &class::func, #class"_"#func

export class DatabaseServerHelper : public DNServer
{

private:

	DatabaseServerHelper() = delete;
	~DatabaseServerHelper() = default;

	DatabaseServerHelper(const DatabaseServerHelper&) = delete;
	// void operator=(const DatabaseServerHelper&) = delete;

	DatabaseServerHelper(DatabaseServerHelper&&) = delete;
	DatabaseServerHelper& operator=(DatabaseServerHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<DatabaseServerHelper>;
	using CVPtr = const Ptr&;

	DNClientProxyHelper::Ptr GetClientProxy()
	{ 
		DNClientProxyHelper::Ptr proxy = GetComponent<DNClientProxyHelper>(EMComponentType::DNClientProxy);
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

	bool InitDatabase()
	{
		if(RdbProxyHelper::CVPtr proxy = GetRdbProxy())
		{
			try
			{
				World::CVPtr pWorld = GetWorld();

				
				std::string* value = pWorld->LaunchParam("connection");
				pqxx::connection check(*value);
				pqxx::nontransaction checkTxn(check);

				if (std::string* names = pWorld->LaunchParam("dbnames"))
				{
					std::vector<std::string> dbNames = StrSplit(*names, ",");
					
					for (std::string& dbName : dbNames)
					{
						try
						{
							EnumName<EMSqlDbNameEnum>(dbName); // check vaild
						}
						catch(...)
						{
							return false;
						}

						if (!checkTxn.query_value<bool>(std::format("SELECT EXISTS (SELECT 1 FROM pg_database WHERE datname = '{}');", dbName)))
						{
							checkTxn.exec(std::format("CREATE DATABASE \"{}\";", dbName));
							GetLogger()->Record(ELogLevel_Debug, "Create Database:{}", dbName);
						}

						uint16_t key = (uint16_t)EnumName<EMSqlDbNameEnum>(dbName);
						std::string connectStr = std::format("{} dbname = {}", *value, dbName);

						auto connection = std::make_shared<pqxx::connection>(connectStr);

						proxy->AddConnection(key, std::move(connection));
					}
				}
				else
				{

					return false;
				}

				std::unordered_map<EMSqlDbNameEnum, std::vector<Message*> > registTable = {
					{
						EMSqlDbNameEnum::Account,
						{
							(Message*)GDb::Account::internal_default_instance(),
						}
					},
					{
						EMSqlDbNameEnum::Nightmare,
						{
							(Message*)GDb::Player::internal_default_instance(),
						}
					},
				};

				GDb::SingleTon kv;
				std::string schemaMd5;

				for (auto& [dbNameEnum, dbEntitys] : registTable)
				{
					if (auto connection = proxy->GetConnection((uint16_t)dbNameEnum))
					{
						pqxx::work txn(*connection);
						DbSqlHelper<GDb::SingleTon> singleTon(&txn);
						singleTon.InitEntity(kv);

						if (!singleTon.IsExist())
						{
							GetLogger()->Record(ELogLevel_Debug, "Create Table:SingleTon");
							singleTon.CreateTable().Commit();
						}

						for (Message* dbEntity : dbEntitys)
						{
							DbSqlHelper<Message> helper(&txn, dbEntity);

							const std::string& tableName = helper.GetName();
							kv.set_key(std::format("{}_Schema", tableName));
							schemaMd5 = helper.GetTableSchemaMd5();

							if (!helper.IsExist())
							{

								GetLogger()->Record(ELogLevel_Debug, "Create Table:{}", tableName);
								helper.CreateTable().Commit();

								kv.set_value(schemaMd5);
								singleTon.Insert().Commit();
								continue;
							}

							// #define DBSelectOne(obj, name) .SelectOne(#name, [&obj]() { return obj.name(); })
							// #define DBSelectCond(obj, name, cond, splicing) .SelectCond(#name, cond, splicing, [&obj]() { return obj.name(); })
							// #define DBUpdate(obj, name) .Update(obj, #name, [&obj]() { return obj.name(); })
							// #define DBUpdateCond(obj, name, cond, splicing) .UpdateCond(#name, cond, splicing, [&obj]() { return obj.name(); })
							// #define DBDeleteCond(obj, name, cond, splicing) .DeleteCond(#name, cond, splicing, [&obj]() { return obj.name(); })

							// // key method
							// #define DBUpdateByKey(obj, name) .UpdateByKey(#name, [&obj]() { return obj.name(); })
							// #define DBSelectByKey(obj, name) .SelectByKey(#name, [&obj]() { return obj.name(); })

							#define DBSelectByKey(obj, name) .SelectByKey(#name, [&obj]() { return obj.name(); })

							singleTon
								DBSelectByKey(kv, key)
								.Commit();

							if (!singleTon.IsSuccess() || !singleTon.Result().size())
							{
								continue;
							}

							kv = *singleTon.Result()[0];

							if (schemaMd5 != kv.value())
							{
								GetLogger()->Record(ELogLevel_Debug, "not match md5:\n{}\n{}", schemaMd5, kv.value());
								helper.UpdateTable().Commit();


								kv.set_value(schemaMd5);
								singleTon.UpdateByKey("key").Commit();
							}
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

		if (DNClientProxy::CVPtr proxy = GetComponent<DNClientProxy>(EMComponentType::DNClientProxy))
		{
			proxy->onConnection = [this,msgHandle](DNSocketChannel::CVPtr channel)
				{
					DNClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}

					const std::string& peeraddr = channel->peeraddr();

					if (channel->isConnected())
					{
						GetLogger()->Record(EL10nCode_SrvConnOn, peeraddr, channel->fd(), channel->id());

						channel->SetWorld(GetWorldW());
						
						proxyHelper->SetRegistEvent(msgHandle->GetClientRegistFunc());
						TickMainSpaceDll(proxyHelper.get(), FUNCPLACE(DNClientProxy,InitConnectedChannel),  channel);
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

								proxyHelper->Timer()->setTimeout(200, [this, originIp, originPort](uint64_t timerID)
									{
										DNClientProxyHelper::CVPtr proxyHelper = GetClientProxy();
										if(!proxyHelper){ return ;}
										TickMainSpaceDll(proxyHelper.get(), FUNCPLACE(DNClientProxy,RedirectClient),  std::stoi(originPort), originIp);
									});
							}
						}

						proxyHelper->SetRegistType(0);
					}

					if (proxyHelper->isReconnect())
					{
					}
				};

			proxy->onMessage = [this,msgHandle](DNSocketChannel::CVPtr channel, hv::Buffer* buf)
				{
					DNClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

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
						if (DNTask<Message*>* task = proxyHelper->GetMsg(packet->msgId)) // client sock request
						{
							proxyHelper->DelMsg(packet->msgId);
							task->Resume();

							if (Message* message = task->GetResult())
							{
								if (!message->ParseFromString(msgData))
								{
									task->SetFlag(EMDNTaskFlag::PaserError);
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

		return InitDatabase();
	}

	int HandleServerShutdown()
	{
		if (DNClientProxyHelper::CVPtr proxy = GetClientProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;
			proxy->SetRegistEvent(nullptr);

			proxy->MsgMapClear();
		}

		return true;
	}

};
