module;
export module LogicMessage:LogicDedicated;

import DNTask;
import FuncHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ClientEntityManagerHelper;
import std.compat;
import DNSocketProxy;
import LogicServerHelper;
import ECSW;

namespace LogicMessage
{
	export DNTaskVoid Msg_ReqLoadEntityData(const World::Ptr& world, const DNSocketProxy::Ptr& channel, uint32_t msgId, std::string binMsg)
	{
		GMsg::d2L_ReqLoadEntityData request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}
		GMsg::L2d_ResLoadEntityData response;

		FinalExecute final([&response, msgId, channel](){
			std::string binData;
			response.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Res, "", binData, channel);
		});

		GDb::Player player;
		if (!player.ParseFromString(request.entity_data()))
		{
			co_return;
		}

		LogicServerHelper::Ptr dnServer = world->GetSystem<LogicServerHelper>(EMSystemType::DNServer);
		ClientEntityManagerHelper::Ptr entityMan = dnServer->GetClientEntityManager();

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

		co_return;
	}

	export void Msg_ReqSaveEntityData(const World::Ptr& world, const DNSocketProxy::Ptr& channel, std::string binMsg)
	{
		GMsg::d2L_ReqSaveEntityData request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		GDb::Player player;
		
		LogicServerHelper::Ptr dnServer = world->GetSystem<LogicServerHelper>(EMSystemType::DNServer);

		if (!player.ParseFromString(request.entity_data()))
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "Save data but parse error!");
			return;
		}

		
		ClientEntityManagerHelper::Ptr entityMan = dnServer->GetClientEntityManager();
		ClientEntity::Ptr entity = entityMan->GetEntity(player.account_id());

		if(!entity)
		{
			return;
		}

		if (!entity)
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "ReqSaveData not entity!");
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
			dnServer->GetLogger()->Record(ELogLevel_Debug, "SaveData but dbEntity is null!");
		}

	}
}