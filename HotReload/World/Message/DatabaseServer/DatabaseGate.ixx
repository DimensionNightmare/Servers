export module DatabaseServerMessage:DatabaseGate;

import DbUtils;
import DatabaseServerHelper;
import ThirdParty.Libhv;
import FuncHelper;
import std.compat;
import ThirdParty.PbGen;
import Logger;
import MessagePack;
import DatabaseServerMessage;
import ThirdParty.Protobuf;

namespace MsgHandleRegister
{

	HandleRegistry<GMsg::L2D_ReqLoadData, GMsg::D2L_ResLoadData, EMMsgDeal::Req> Exe_ReqLoadData =
				[](auto request, auto response, const SocketChannel::Ptr& channel)
	{
		
		DatabaseServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<DatabaseServerHelper>(EMSystemType::Server);

		if (auto connection = dnServer->GetRdbProxy()->GetConnection(EMSqlDbNameEnum::Nightmare))
		{
			auto dealFunc = [&](Message* findMsg, const FieldDescriptor* field)
				{
					findMsg->ParseFromString(request->entitydata());

					pqxx::work txn(*connection);
					DbSqlHelper dbHelper(&txn, channel->GetWorld(), findMsg);

					auto query = [&]()
						{
							

							dbHelper
								.SelectByKey(field)
								.Limit(request->limit())
								.Commit();

							if (int64_t resSize = dbHelper.Result().size())
							{
								for (int64_t cur = 0; cur < resSize; cur++)
								{
									std::string* binData = response->add_entitydata();
									dbHelper.Result()[cur]->SerializeToString(binData);
								}

							}
						};

					query();

					// not exist just create
					if (!response->entitydata_size() && request->needcreate())
					{
						dbHelper.Insert(true).Commit();

						if (dbHelper.IsSuccess())
						{
							query();
						}

						txn.commit();
					}
				};

			if (const Descriptor* descriptor = PbGen::FindMessageTypeByName(request->tablename()))
			{
				if (const Message* prototype = PbGen::GetPrototype(descriptor))
				{
					const FieldDescriptor* field = descriptor->FindFieldByNumber(request->keynumber());
					if (!field)
					{
						response->set_errorcode(EL10nCode_UnkonwOpreator);
						return;
					}

					Message* message = prototype->New();

					try
					{
						dealFunc(message, field);
					}
					catch (const std::exception& e)
					{
						LoggerPrint::Log(channel, ELogLevel_Debug, "{}", e.what());
						response->set_errorcode(EL10nCode_UnkonwOpreator);
					}

					delete message;
				}
				else
				{
					response->set_errorcode(EL10nCode_PBMessageNotGen);
				}
			}
			else
			{
				response->set_errorcode(EL10nCode_PBMessageNotExist);

			}

		}
		else
		{
			response->set_errorcode(EL10nCode_DBNotConnect);
		}
	};

	HandleRegistry<GMsg::L2D_ReqSaveData, GMsg::D2L_ResSaveData, EMMsgDeal::Req> Exe_ReqSaveData =
				[](auto request, auto response, const SocketChannel::Ptr& channel)
	{
		DatabaseServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<DatabaseServerHelper>(EMSystemType::Server);

		if (auto connection = dnServer->GetRdbProxy()->GetConnection(EMSqlDbNameEnum::Nightmare))
		{
			auto dealFunc = [&](Message* findMsg)
				{
					findMsg->ParseFromString(request->entitydata());

					pqxx::work txn(*connection);
					DbSqlHelper dbHelper(&txn, channel->GetWorld(), findMsg);

					dbHelper
						.UpdateByKey(request->keynumber())
						.Commit();

					txn.commit();

				};

			if (const Descriptor* descriptor = PbGen::FindMessageTypeByName(request->tablename()))
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
						LoggerPrint::Log(channel, ELogLevel_Debug, "{}", e.what());
						response->set_errorcode(EL10nCode_UnkonwOpreator);
					}

					delete message;
				}
				else
				{
					response->set_errorcode(EL10nCode_PBMessageNotGen);
				}
			}
			else
			{
				response->set_errorcode(EL10nCode_PBMessageNotExist);

			}

		}
		else
		{
			response->set_errorcode(EL10nCode_DBNotConnect);
		}
	};
}

