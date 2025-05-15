module;
export module LogicMessage:LogicDedicated;

import DNTask;
import FuncHelper;
import LogicServerHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ClientEntityManagerHelper;
import std.compat;

namespace LogicMessage
{
	export DNTaskVoid Msg_ReqLoadEntityData(hv::SocketChannelPtr channel, uint32_t msgId, std::string binMsg)
	{
		GMsg::d2L_ReqLoadEntityData request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}
		GMsg::L2d_ResLoadEntityData response;

		GDb::Player player;
		if (!player.ParseFromString(request.entity_data()))
		{
			co_return;
		}

		LogicServerHelper* dnServer = GetLogicServer();
		ClientEntityManagerHelper* entityMan = dnServer->GetClientEntityManager();

		ClientEntity::Ptr entity = entityMan->GetEntity(player.account_id());

		if (!entity)
		{
			response.set_state_code(1);
		}
		else
		{
			
			co_await entityMan->LoadEntityData(entity, &request, &response);
			std::string* entity_data = response.add_entity_data();
			entity->GetDbEntity()->SerializeToString(entity_data);

		}

		std::string binData;
		response.SerializeToString(&binData);
		MessagePackAndSend(msgId, EMMsgDeal::Res, "", binData, channel);

		co_return;
	}

	export void Msg_ReqSaveEntityData(hv::SocketChannelPtr channel, std::string binMsg)
	{
		GMsg::d2L_ReqSaveEntityData request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		GDb::Player player;
		if (!player.ParseFromString(request.entity_data()))
		{
			SPidLogger.Record(ELogLevel_Debug, "Save data but parse error!");
			return;
		}

		LogicServerHelper* dnServer = GetLogicServer();
		ClientEntityManagerHelper* entityMan = dnServer->GetClientEntityManager();
		ClientEntity::Ptr entity = entityMan->GetEntity(player.account_id());

		if(!entity)
		{
			return;
		}

		if (!entity)
		{
			SPidLogger.Record(ELogLevel_Debug, "ReqSaveData not entity!");
			return;
		}

		if (GDb::Player* dbEntity = entity->GetDbEntity())
		{
			dbEntity->MergeFrom(player);
			if (request.runtime_save())
			{
				entity->SetFlag(EMClientEntityFlag::DBModify);
			}
			
		}
		else
		{
			SPidLogger.Record(ELogLevel_Debug, "SaveData but dbEntity is null!");
		}

	}
}