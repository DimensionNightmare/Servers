export module LogicServerMessage:LogicDedicated;

import LogicServerHelper;
import Task;
import ThirdParty.PbGen;
import Logger;
import LogicServerMessage;

namespace MsgHandleRegister
{

	HandleRegistry<GMsg::d2L_ReqLoadEntityData, GMsg::L2d_ResLoadEntityData, EMMsgDeal::Req> Msg_ReqAuthToken =
				[](auto request, auto response, SocketChannel::Ptr channel) -> TaskVoid
	{
		
		GDb::Player player;
		if (!player.ParseFromString(request->entitydata()))
		{
			co_return;
		}

		LogicServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);
		ClientEntityManagerHelper::Ptr entityMan = dnServer->GetClientEntityManager();

		ClientEntityHelper::Ptr entity = entityMan->GetEntity(player.accountid());

		if (!entity)
		{
			response->set_errorcode(EL10nCode_NoneClientEntity);
		}
		else
		{
			
			bool success = co_await entityMan->LoadEntity(entity, request, response);

		}

		co_return;
	};

	HandleRegistry<GMsg::d2L_ReqSaveEntityData, void, EMMsgDeal::Ret> Msg_ReqSaveEntityData =
				[](auto request, SocketChannel::Ptr channel)
	{
		
		GDb::Player player;
		
		LogicServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);

		if (!player.ParseFromString(request->entitydata()))
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "Save data but parse error!");
			return;
		}

		
		ClientEntityManagerHelper::Ptr entityMan = dnServer->GetClientEntityManager();
		ClientEntity::Ptr entity = entityMan->GetEntity(player.accountid());

		if(!entity)
		{
			return;
		}

		if (!entity)
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "ReqSaveData not entity!");
			return;
		}

		if (GDb::Player* dbEntity = entity->GetDbEntity())
		{
			dbEntity->MergeFrom(player);
			if (request->runtimesave())
			{
				entity->SetFlag(EMClientEntityFlag::DBModify);
			}
			
		}
		else
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "SaveData but dbEntity is null!");
		}
	};
}