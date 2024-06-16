module;
#include <coroutine>
#include <string>
#include <cstdint>

#include "StdMacro.h"
export module DatabaseMessage:DatabaseGate;

import MessagePack;
import DatabaseServerHelper;
import Logger;
import DbUtils;
import MessagePack;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ThirdParty.Libpqxx;

namespace DatabaseMessage
{

	export void Exe_ReqLoadData(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		L2D_ReqLoadData* request = reinterpret_cast<L2D_ReqLoadData*>(msg);
		D2L_ResLoadData response;

		DatabaseServerHelper* dnServer = GetDatabaseServer();

		string binData;

		if (pq_connection* conn = dnServer->GetSqlProxy(SqlDbNameEnum::Nightmare))
		{
			auto dealFunc = [&](Message* findMsg)
				{
					findMsg->ParseFromString(request->entity_data());

					pq_work txn(*conn);
					DbSqlHelper dbHelper(&txn, findMsg);

					auto query = [&]()
						{
							dbHelper
								.SelectByKey(request->key_name().c_str())
								.Limit(request->limit())
								.Commit();

							if (int resSize = dbHelper.Result().size())
							{
								for (int cur = 0; cur < resSize; cur++)
								{
									string* binData = response.add_entity_data();
									dbHelper.Result()[cur]->SerializeToString(binData);
								}

							}
						};

					query();

					// not exist just create
					if (!response.entity_data_size() && request->need_create())
					{
						dbHelper.Insert(true).Commit();

						if (dbHelper.IsSuccess())
						{
							query();
						}

						txn.commit();
					}
				};

			if (const Descriptor* descriptor = PB_FindMessageTypeByName(request->table_name()))
			{
				if (const Message* prototype = PB_GetPrototype(descriptor))
				{
					Message* message = prototype->New();

					try
					{
						dealFunc(message);
					}
					catch (const exception& e)
					{
						DNPrint(0, LoggerLevel::Debug, e.what());
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
		MessagePack(msgId, MsgDeal::Res, nullptr, binData);

		channel->write(binData);
	}

	export void Exe_ReqSaveData(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		L2D_ReqSaveData* request = reinterpret_cast<L2D_ReqSaveData*>(msg);
		D2L_ResSaveData response;

		DatabaseServerHelper* dnServer = GetDatabaseServer();

		string binData;

		if (pq_connection* conn = dnServer->GetSqlProxy(SqlDbNameEnum::Nightmare))
		{
			auto dealFunc = [&](Message* findMsg)
				{
					findMsg->ParseFromString(request->entity_data());

					pq_work txn(*conn);
					DbSqlHelper dbHelper(&txn, findMsg);

					dbHelper
						.UpdateByKey(request->key_name().c_str())
						.Commit();

					txn.commit();

				};

			if (const Descriptor* descriptor = PB_FindMessageTypeByName(request->table_name()))
			{
				if (const Message* prototype = PB_GetPrototype(descriptor))
				{
					Message* message = prototype->New();

					try
					{
						dealFunc(message);
					}
					catch (const exception& e)
					{
						DNPrint(0, LoggerLevel::Debug, e.what());
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
		MessagePack(msgId, MsgDeal::Res, nullptr, binData);

		channel->write(binData);
	}
}