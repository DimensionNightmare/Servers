export module DatabaseServerMessage:DatabaseGate;

import DbUtils;
import DatabaseServerHelper;

import ThirdParty.Libhv;
import FuncHelper;
import std.compat;
import ThirdParty.PbGen;

export namespace DatabaseServerMessage
{

	void Exe_ReqLoadData(SocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::L2D_ReqLoadData request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		GMsg::D2L_ResLoadData response;

		FinalExecute final([&response, msgId, channel](){
			std::string binData;
			response.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Res, binData, channel);
		});

		DatabaseServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<DatabaseServerHelper>(EMSystemType::Server);

		if (auto connection = dnServer->GetRdbProxy()->GetConnection(EMSqlDbNameEnum::Nightmare))
		{
			auto dealFunc = [&](Message* findMsg)
				{
					findMsg->ParseFromString(request.entitydata());

					pqxx::work txn(*connection);
					DbSqlHelper dbHelper(&txn, dnServer->GetLogger(), findMsg);

					auto query = [&]()
						{
							dbHelper
								// .SelectByKey(request.keyname())
								.Limit(request.limit())
								.Commit();

							if (int64_t resSize = dbHelper.Result().size())
							{
								for (int64_t cur = 0; cur < resSize; cur++)
								{
									std::string* binData = response.add_entitydata();
									dbHelper.Result()[cur]->SerializeToString(binData);
								}

							}
						};

					query();

					// not exist just create
					if (!response.entitydata_size() && request.needcreate())
					{
						dbHelper.Insert(true).Commit();

						if (dbHelper.IsSuccess())
						{
							query();
						}

						txn.commit();
					}
				};

			if (const Descriptor* descriptor = PbGen::FindMessageTypeByName(request.tablename()))
			{
				if (const Message* prototype = PbGen::GetPrototype(descriptor))
				{
					Message* message = prototype->New();

					try
					{
						dealFunc(message);
					}
					catch (const std::exception& e)
					{
						dnServer->GetLogger()->Record(ELogLevel_Debug, "{}", e.what());
						response.set_errorcode(EL10nCode_UnkonwOpreator);
					}

					delete message;
				}
				else
				{
					response.set_errorcode(EL10nCode_PBMessageNotGen);
				}
			}
			else
			{
				response.set_errorcode(EL10nCode_PBMessageNotExist);

			}

		}
		else
		{
			response.set_errorcode(EL10nCode_DBNotConnect);
		}
	}

	void Exe_ReqSaveData(SocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::L2D_ReqSaveData request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		GMsg::D2L_ResSaveData response;

		FinalExecute final([&response, msgId, channel](){
			std::string binData;
			response.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Res, binData, channel);
		});

		DatabaseServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<DatabaseServerHelper>(EMSystemType::Server);

		std::string binData;

		if (auto connection = dnServer->GetRdbProxy()->GetConnection(EMSqlDbNameEnum::Nightmare))
		{
			auto dealFunc = [&](Message* findMsg)
				{
					findMsg->ParseFromString(request.entitydata());

					pqxx::work txn(*connection);
					DbSqlHelper dbHelper(&txn, dnServer->GetLogger(), findMsg);

					dbHelper
						.UpdateByKey(request.keynumber())
						.Commit();

					txn.commit();

				};

			if (const Descriptor* descriptor = PbGen::FindMessageTypeByName(request.tablename()))
			{
				if (const Message* prototype = PbGen::GetPrototype(descriptor))
				{
					Message* message = prototype->New();

					try
					{
						dealFunc(message);
					}
					catch (const std::exception& e)
					{
						dnServer->GetLogger()->Record(ELogLevel_Debug, "{}", e.what());
						response.set_errorcode(EL10nCode_UnkonwOpreator);
					}

					delete message;
				}
				else
				{
					response.set_errorcode(EL10nCode_PBMessageNotGen);
				}
			}
			else
			{
				response.set_errorcode(EL10nCode_PBMessageNotExist);

			}

		}
		else
		{
			response.set_errorcode(EL10nCode_DBNotConnect);
		}

	}

}