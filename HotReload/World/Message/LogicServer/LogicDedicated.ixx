export module LogicServerMessage:LogicDedicated;

import LogicServerHelper;
import std;
import ThirdParty.Libhv;
import FuncHelper;
import Task;
import ThirdParty.PbGen;

export namespace LogicServerMessage
{
	TaskVoid Msg_ReqLoadEntityData(SocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
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
			MessagePackAndSend(msgId, EMMsgDeal::Res, binData, channel);
		});

		GDb::Player player;
		if (!player.ParseFromString(request.entity_data()))
		{
			co_return;
		}

		LogicServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);
		ClientEntityManagerHelper::CVPtr entityMan = dnServer->GetClientEntityManager();

		ClientEntity::CVPtr entity = entityMan->GetEntity(player.account_id());

		if (!entity)
		{
			response.set_error_code(EL10nCode_NoneClientEntity);
		}
		else
		{
			
			co_await entityMan->LoadEntity(entity->GetSelf<ClientEntityHelper>(), &request, &response);

		}

		co_return;
	}

	void Msg_ReqSaveEntityData(SocketChannel::CVPtr channel, const std::string& binMsg)
	{
		GMsg::d2L_ReqSaveEntityData request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		GDb::Player player;
		
		LogicServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);

		if (!player.ParseFromString(request.entity_data()))
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "Save data but parse error!");
			return;
		}

		
		ClientEntityManagerHelper::CVPtr entityMan = dnServer->GetClientEntityManager();
		ClientEntity::CVPtr entity = entityMan->GetEntity(player.account_id());

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