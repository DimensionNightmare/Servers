module;
export module DatabaseMessage:DatabaseGate;

import FuncHelper;
import Logger;
import DbUtils;
import FuncHelper;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ThirdParty.Libpqxx;
import std.compat;
import DNServer;
import DatabaseServerHelper;
import ECSW;

namespace DatabaseMessage
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
						dnServer->GetLogger()->Record(ELogLevel_Debug, e.what());
						response.set_state_code(5);
					}

					delete message;
				}
				else
				{
					response.set_state_code(6);
				}
			}
			else
			{
				response.set_state_code(4);

			}

		}
		else
		{
			response.set_state_code(3);
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
						dnServer->GetLogger()->Record(ELogLevel_Debug, e.what());
						response.set_state_code(5);
					}

					delete message;
				}
				else
				{
					response.set_state_code(6);
				}
			}
			else
			{
				response.set_state_code(4);

			}

		}
		else
		{
			response.set_state_code(3);
		}

	}
}