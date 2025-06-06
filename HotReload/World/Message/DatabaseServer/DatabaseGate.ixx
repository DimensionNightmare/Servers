module;
export module DatabaseServerMessage:DatabaseGate;

import DbUtils;
import DatabaseServerHelper;

import ThirdParty.Libhv;
import FuncHelper;
import std.compat;

namespace DatabaseServerMessage
{

	export void Exe_ReqLoadData(const DNSocketChannel::Ptr& channel, uint32_t msgId, const std::string& binMsg)
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

		DatabaseServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<DatabaseServerHelper>(EMSystemType::DNServer);

		if (auto connection = dnServer->GetRdbProxy()->GetConnection(static_cast<uint16_t>(EMSqlDbNameEnum::Nightmare)))
		{
			auto dealFunc = [&](Message* findMsg)
				{
					findMsg->ParseFromString(request.entity_data());

					pqxx::work txn(*connection);
					DbSqlHelper dbHelper(&txn, findMsg);

					auto query = [&]()
						{
							dbHelper
								.SelectByKey(request.key_name())
								.Limit(request.limit())
								.Commit();

							if (int64_t resSize = dbHelper.Result().size())
							{
								for (int64_t cur = 0; cur < resSize; cur++)
								{
									std::string* binData = response.add_entity_data();
									dbHelper.Result()[cur]->SerializeToString(binData);
								}

							}
						};

					query();

					// not exist just create
					if (!response.entity_data_size() && request.need_create())
					{
						dbHelper.Insert(true).Commit();

						if (dbHelper.IsSuccess())
						{
							query();
						}

						txn.commit();
					}
				};

			if (const Descriptor* descriptor = PbGen::FindMessageTypeByName(request.table_name()))
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
						response.set_error_code(EL10nCode_UnkonwOpreator);
					}

					delete message;
				}
				else
				{
					response.set_error_code(EL10nCode_PBMessageNotGen);
				}
			}
			else
			{
				response.set_error_code(EL10nCode_PBMessageNotExist);

			}

		}
		else
		{
			response.set_error_code(EL10nCode_DBNotConnect);
		}
	}

	export void Exe_ReqSaveData(const DNSocketChannel::Ptr& channel, uint32_t msgId, const std::string& binMsg)
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

		DatabaseServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<DatabaseServerHelper>(EMSystemType::DNServer);

		std::string binData;

		if (auto connection = dnServer->GetRdbProxy()->GetConnection(static_cast<uint16_t>(EMSqlDbNameEnum::Nightmare)))
		{
			auto dealFunc = [&](Message* findMsg)
				{
					findMsg->ParseFromString(request.entity_data());

					pqxx::work txn(*connection);
					DbSqlHelper dbHelper(&txn, findMsg);

					dbHelper
						.UpdateByKey(request.key_name())
						.Commit();

					txn.commit();

				};

			if (const Descriptor* descriptor = PbGen::FindMessageTypeByName(request.table_name()))
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
						response.set_error_code(EL10nCode_UnkonwOpreator);
					}

					delete message;
				}
				else
				{
					response.set_error_code(EL10nCode_PBMessageNotGen);
				}
			}
			else
			{
				response.set_error_code(EL10nCode_PBMessageNotExist);

			}

		}
		else
		{
			response.set_error_code(EL10nCode_DBNotConnect);
		}

	}
}