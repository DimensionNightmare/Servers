module;
export module DatabaseMessage:DatabaseGate;

import FuncHelper;
import DatabaseServerHelper;
import Logger;
import DbUtils;
import FuncHelper;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ThirdParty.Libpqxx;
import std.compat;

namespace DatabaseMessage
{

	export void Exe_ReqLoadData(hv::SocketChannelPtr channel, uint32_t msgId, std::string binMsg)
	{
		GMsg::L2D_ReqLoadData request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		GMsg::D2L_ResLoadData response;

		DatabaseServerHelper* dnServer = GetDatabaseServer();

		std::string binData;

		if (pqxx::connection* conn = dnServer->GetSqlProxy(EMSqlDbNameEnum::Nightmare))
		{
			auto dealFunc = [&](Message* findMsg)
				{
					findMsg->ParseFromString(request.entity_data());

					pqxx::work txn(*conn);
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
						SPidLogger.Record(ELogLevel_Debug, e.what());
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

		response.SerializeToString(&binData);

		MessagePackAndSend(msgId, EMMsgDeal::Res, "", binData, channel);
	}

	export void Exe_ReqSaveData(hv::SocketChannelPtr channel, uint32_t msgId, std::string binMsg)
	{
		GMsg::L2D_ReqSaveData request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		GMsg::D2L_ResSaveData response;

		DatabaseServerHelper* dnServer = GetDatabaseServer();

		std::string binData;

		if (pqxx::connection* conn = dnServer->GetSqlProxy(EMSqlDbNameEnum::Nightmare))
		{
			auto dealFunc = [&](Message* findMsg)
				{
					findMsg->ParseFromString(request.entity_data());

					pqxx::work txn(*conn);
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
						SPidLogger.Record(ELogLevel_Debug, e.what());
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

		response.SerializeToString(&binData);

		MessagePackAndSend(msgId, EMMsgDeal::Res, "", binData, channel);
	}
}