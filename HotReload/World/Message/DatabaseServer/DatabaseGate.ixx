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
				[](World::Ptr world, SocketChannel::Ptr channel, auto request, auto response)
	{
		
		DatabaseServerHelper::CVPtr dnServer = world->GetSystem<DatabaseServerHelper>(EMSystemType::Server);

		if (auto transaction = dnServer->GetRdbProxy()->GetTransaction(EMSqlDbNameEnum::Nightmare, false))
		{
			auto dealFunc = [&](Message* findMsg, const FieldDescriptor* field)
				{
					findMsg->ParseFromString(request->entitydata());

					DbSqlHelper dbHelper(transaction.get(), world, findMsg);

					auto query = [&]()
						{
							dbHelper
								.SelectByKey(field)
								.Limit(request->limit())
								.Commit();

							const auto& results = dbHelper.GetResult();

							std::ranges::for_each(
								results,
								[response](const auto& result)
								{
									result->SerializeToString(response->add_entitydata());
								}
							);
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

						transaction->commit();
					}
				};

			if (const Descriptor* descriptor = Proto::FindMessageTypeByName(request->tablename()))
			{
				if (const Message* prototype = Proto::GetPrototype(descriptor))
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
						LoggerPrint::Log(world, ELogLevel_Debug, "{}", e.what());
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
				[](World::Ptr world, SocketChannel::Ptr channel, auto request, auto response)
	{
		
		DatabaseServerHelper::CVPtr dnServer = world->GetSystem<DatabaseServerHelper>(EMSystemType::Server);

		if (auto transaction = dnServer->GetRdbProxy()->GetTransaction(EMSqlDbNameEnum::Nightmare, false))
		{
			auto dealFunc = [&](Message* findMsg)
				{
					findMsg->ParseFromString(request->entitydata());

					DbSqlHelper dbHelper(transaction.get(), world, findMsg);

					dbHelper
						.UpdateByKey(request->keynumber())
						.Commit();

					transaction->commit();


					// LoggerPrint::Log(world, ELogLevel_Debug, "Save Data Success! data={}", findMsg->DebugString());
				};

			if (const Descriptor* descriptor = Proto::FindMessageTypeByName(request->tablename()))
			{
				if (const Message* prototype = Proto::GetPrototype(descriptor))
				{
					Message* message = prototype->New();

					try
					{
						dealFunc(message);
					}
					catch (const std::exception& e)
					{
						LoggerPrint::Log(world, ELogLevel_Debug, "{}", e.what());
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

