module;
#include <coroutine>
#include <cstdint>
#include <list>
#include "hv/Channel.h"

#include "StdMacro.h"
#include "Server/S_Dedicated.pb.h"
#include "GDef/GDef.pb.h"
export module LogicMessage:LogicDedicated;

import DNTask;
import MessagePack;
import LogicServerHelper;
import Logger;

using namespace std;
using namespace google::protobuf;
using namespace hv;
using namespace GMsg;

namespace LogicMessage
{
    export DNTaskVoid Msg_ReqLoadData(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		d2D_ReqLoadData* request = reinterpret_cast<d2D_ReqLoadData*>(msg);
		D2d_ResLoadData response;

		GDb::Player player;
		if(!player.ParseFromString(request->entity_data()))
		{
			co_return;
		}
		
		LogicServerHelper* dnServer = GetLogicServer();
		ClientEntityManagerHelper* entityMan = dnServer->GetClientEntityManager();

		ClientEntity* entity = entityMan->GetEntity(player.account_id());
		if(!entity)
		{
			response.set_state_code(1);
		}
		else
		{
			co_await entityMan->LoadEntityData(entity, request, &response);
		}

		string binData;
		response.SerializeToString(&binData);
		MessagePack(msgId, MsgDeal::Res, nullptr, binData);
		channel->write(binData);

		co_return;
	}

	export void Msg_ReqSaveData(SocketChannelPtr channel, Message* msg)
	{
		d2D_ReqSaveData* request = reinterpret_cast<d2D_ReqSaveData*>(msg);

		GDb::Player player;
		if(!player.ParseFromString(request->entity_data()))
		{
			DNPrint(0, LoggerLevel::Debug, "Save data but parse error!");
			return;
		}

        LogicServerHelper* dnServer = GetLogicServer();
		ClientEntityManagerHelper* entityMan = dnServer->GetClientEntityManager();
        ClientEntity* entity = entityMan->GetEntity(player.account_id());

        if(!entity)
		{
            DNPrint(0, LoggerLevel::Debug, "ReqSaveData not entity!");
			return;
		}

        if(GDb::Player* dbEntity = entity->GetDbEntity())
        {
            dbEntity->MergeFrom(player);
			if(request->runtime_save())
			{
            	entity->SetFlag(ClientEntityFlag::DBModify);
				entity->ClearFlag(ClientEntityFlag::DBModifyPartial);
			}
			else
			{
				entity->SetFlag(ClientEntityFlag::DBModifyPartial);
			}
        }
        else
        {
            DNPrint(0, LoggerLevel::Debug, "SaveData but dbEntity is null!");
        }
	}
}