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

		LogicServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);
		ClientEntityManagerHelper::CVPtr entityMan = dnServer->GetClientEntityManager();

		ClientEntityHelper::CVPtr entity = entityMan->GetEntity(player.accountid());

		if (!entity)
		{
			response->set_errorcode(EL10nCode_NoneClientEntity);
		}
		else
		{
			
			bool success = co_await entityMan->LoadEntity(entity, request, response);
			if (!success)
			{
				LoggerPrint::Log(channel, ELogLevel_Debug, "Load Entity d2L_ReqLoadEntityData error id = {}!", player.accountid());
			}

		}

		co_return;
	};

	HandleRegistry<GMsg::d2L_ReqSaveEntityData, void, EMMsgDeal::Ret> Msg_ReqSaveEntityData =
				[](auto request, SocketChannel::CVPtr channel)
	{
		
		GDb::Player player;
		
		LogicServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);

		if (!player.ParseFromString(request->entitydata()))
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "Save data but parse error!");
			return;
		}

		
		ClientEntityManagerHelper::CVPtr entityMan = dnServer->GetClientEntityManager();
		ClientEntity::CVPtr entity = entityMan->GetEntity(player.accountid());

		if (!entity)
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "ReqSaveData not entity!");
			return;
		}

		if (GDb::Player* dbEntity = entity->GetDbEntity())
		{
			dbEntity->MergeFrom(player);
			// if (request->runtimesave())
			{
				entity->SetFlag(EMClientEntityFlag::DBModify);
			}

			// if(!entityMan->GetSaveTimerId())
			// {
			// 	size_t timerId = entityMan->GetTimer()->SetTimeout(5000, [entityMan](size_t)
			// 	{
			// 		entityMan->CheckSaveEntity();
			// 		entityMan->SetSaveTimerId(0);
			// 	});

			// 	entityMan->SetSaveTimerId(timerId);
			// }
			
		}
		else
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "SaveData but dbEntity is null!");
		}
	};

	HandleRegistry<GMsg::S2C_RetAccountReplace, void, EMMsgDeal::Ret> Exe_RetAccountReplace =
				[](auto request, SocketChannel::CVPtr channel)
	{
		
		LogicServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);
		ClientEntityManagerHelper::CVPtr entityMan = dnServer->GetClientEntityManager();

		ClientEntityHelper::CVPtr entity = entityMan->GetEntity(request->accountid());
		if (!entity)
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "Client Entity Kick Not Exist !");
			return;
		}

		RoomEntityManagerHelper::CVPtr roomEntityMan = dnServer->GetRoomEntityManager();
		RoomEntityHelper::CVPtr roomEntity = roomEntityMan->GetEntity(entity->RecordRoomId());

		// cache
		if (roomEntity)
		{
			ServerProxyHelper::CVPtr proxyHelper = dnServer->GetServerProxy();
				
			proxyHelper->AddMsg(EMMsgDeal::Ret, request, roomEntity->GetChannel()).Resume();
		}
		else
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "Client Entity Kick Server Not Exist !");
		}

		// close entity save data
		entityMan->RemoveEntity(entity->ID());
	};

	HandleRegistry<GMsg::S2C_ReqGetRoom, GMsg::S2C_ResGetRoom, EMMsgDeal::Redir> Msg_ReqGetRoom =
				[](auto request, auto response, SocketChannel::Ptr channel) -> TaskVoid
	{
		LogicServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);
		ClientEntityManagerHelper::CVPtr entityMan = dnServer->GetClientEntityManager();

		ClientEntityHelper::CVPtr entity = entityMan->GetEntity(request->accountid());
		if (!entity)
		{
			co_return;
		}

		RoomEntityManagerHelper::CVPtr roomEntityMan = dnServer->GetRoomEntityManager();
		RoomEntityHelper::Ptr roomEntity = nullptr;

		// cache
		if (size_t roomId = entity->RecordRoomId())
		{
			roomEntity = roomEntityMan->GetEntity(roomId);
		}

		//pool
		if (!roomEntity)
		{
			size_t mapId = 0;
			// from db
			if(entity->GetDbEntity()->has_mapinfo())
			{
				GDef_MapPointRecord* mapRecord = entity->GetDbEntity()->mutable_mapinfo();
				*mapRecord->mutable_curpoint() = *mapRecord->mutable_lastpoint();

				mapId = mapRecord->curpoint().mapid();
			}
			// new player use default 1

			if(mapId == 0)
			{
				mapId++;

				GDef_MapPointRecord* mapRecord = entity->GetDbEntity()->mutable_mapinfo();
				mapRecord->mutable_curpoint()->set_mapid(mapId);
			}

			auto selects = roomEntityMan->GetEntitysByMapId(mapId)
				| std::views::transform([](const auto& param){
					return param->GetSelf<RoomEntityHelper>();
				});

			if (auto it = std::ranges::min_element(selects, std::greater{}, &RoomEntityHelper::GetConnNum); it != selects.end())
			{
				roomEntity = *it;
			}
			else
			{
				response->set_errorcode(EL10nCode_NotDsServer);
				LoggerPrint::Log(channel, ELogLevel_Debug, "not ds Server");
			}
			
		}

		// req token
		if (roomEntity)
		{
			ServerProxyHelper::CVPtr proxyHelper = dnServer->GetServerProxy();

			// wait data parse
			bool success = co_await proxyHelper->AddMsg(EMMsgDeal::Req, request, roomEntity->GetChannel(), response);
		
			if (!success)
			{
				response->set_errorcode(EL10nCode_ReqRegistTimeout);
			}
			else
			{
				entity->SetRecordRoomId(roomEntity->ID());
				//combin
				response->set_serverip(roomEntity->GetServerIp());
				response->set_serverport(roomEntity->GetServerPort());
			}

		}

		LoggerPrint::Log(channel, ELogLevel_Debug, "ds:{}", response->DebugString());

		co_return;
	};


}