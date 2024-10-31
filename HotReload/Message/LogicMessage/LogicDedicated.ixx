module;
#include "StdMacro.h"
export module LogicMessage:LogicDedicated;

import DNTask;
import FuncHelper;
import LogicServerHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ClientEntityManagerHelper;

namespace LogicMessage
{
	export DNTaskVoid Msg_ReqLoadEntityData(SocketChannelPtr channel, uint32_t msgId,  string binMsg)
	{
		d2L_ReqLoadEntityData request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}
		L2d_ResLoadEntityData response;

		Player player;
		if (!player.ParseFromString(request.entity_data()))
		{
			co_return;
		}

		LogicServerHelper* dnServer = GetLogicServer();
		ClientEntityManagerHelper* entityMan = dnServer->GetClientEntityManager();

		ClientEntity* entity = entityMan->GetEntity(player.account_id());

		if (!entity)
		{
			response.set_state_code(1);
		}
		else
		{
			
			co_await entityMan->LoadEntityData(entity, &request, &response);
			string* entity_data = response.add_entity_data();
			entity->GetDbEntity()->SerializeToString(entity_data);

		}

		string binData;
		response.SerializeToString(&binData);
		MessagePackAndSend(msgId, EMMsgDeal::Res, nullptr, binData, channel);

		co_return;
	}

	export void Msg_ReqSaveEntityData(SocketChannelPtr channel, string binMsg)
	{
		d2L_ReqSaveEntityData request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		Player player;
		if (!player.ParseFromString(request.entity_data()))
		{
			DNPrint(0, EMLoggerLevel::Debug, "Save data but parse error!");
			return;
		}

		LogicServerHelper* dnServer = GetLogicServer();
		ClientEntityManagerHelper* entityMan = dnServer->GetClientEntityManager();
		ClientEntity* entity = entityMan->GetEntity(player.account_id());

		if(!entity)
		{
			return;
		}

		if (!entity)
		{
			DNPrint(0, EMLoggerLevel::Debug, "ReqSaveData not entity!");
			return;
		}

		if (Player* dbEntity = entity->GetDbEntity())
		{
			dbEntity->MergeFrom(player);
			if (request.runtime_save())
			{
				entity->SetFlag(EMClientEntityFlag::DBModify);
			}
			
		}
		else
		{
			DNPrint(0, EMLoggerLevel::Debug, "SaveData but dbEntity is null!");
		}

	}
}