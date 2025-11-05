export module LogicServerMessage:LogicRedirect;

import LogicServerHelper;
import FuncHelper;
import ThirdParty.PbGen;
import Logger;
import LogicServerMessage;

namespace MsgHandleRegister
{

	HandleRegistry<GMsg::S2C_RetAccountReplace, void, EMMsgDeal::Ret> Exe_RetAccountReplace =
				[](auto request, SocketChannel::Ptr channel)
	{
		
		LogicServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);
		ClientEntityManagerHelper::Ptr entityMan = dnServer->GetClientEntityManager();

		ClientEntityHelper::Ptr entity = entityMan->GetEntity(request->accountid());
		if (!entity)
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "Client Entity Kick Not Exist !");
			return;
		}

		RoomEntityManagerHelper::Ptr roomEntityMan = dnServer->GetRoomEntityManager();
		RoomEntityHelper::Ptr roomEntity = roomEntityMan->GetEntity(entity->RecordRoomId());

		// cache
		if (roomEntity)
		{
			MessagePackAndSend(0, EMMsgDeal::Ret, request, roomEntity->GetChannel());
		}
		else
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "Client Entity Kick Server Not Exist !");
		}

		// close entity save data
		entityMan->RemoveEntity(entity->ID());
	};

	HandleRegistry<GMsg::C2S_ReqAuthToken, GMsg::S2C_ResAuthToken, EMMsgDeal::Redir> Msg_ReqClientLogin =
				[](auto request, auto response, SocketChannel::Ptr channel) -> TaskVoid
	{
		LogicServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);
		ClientEntityManagerHelper::Ptr entityMan = dnServer->GetClientEntityManager();

		ClientEntityHelper::Ptr entity = entityMan->AddEntity(request->accountid());
		if (entity)
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "AddEntity Client!");

			// msg will destroy. MessageHandle not will waiting.
			bool success = co_await entityMan->LoadEntity(entity, nullptr, nullptr);

			if(!success)
			{
				LoggerPrint::Log(channel, ELogLevel_Debug, "Load Database Error!");
				
				co_return;
			}
			else
			{
				if (!entity->HasFlag(EMClientEntityFlag::DBInited))
				{
					LoggerPrint::Log(channel, ELogLevel_Debug, "AddEntity Client but not from db!");
				}
			}
		}
		else
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "AddEntity Exist Client!");
			entity = entityMan->GetEntity(request->accountid());
		}

		RoomEntityManagerHelper::Ptr roomEntityMan = dnServer->GetRoomEntityManager();
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
			else
			{
				mapId++;

				GDef_MapPointRecord* mapRecord = entity->GetDbEntity()->mutable_mapinfo();
				mapRecord->mutable_curpoint()->set_mapid(mapId);
			}

			std::list<RoomEntity::Ptr> roomEntityList = roomEntityMan->GetEntitysByMapId(mapId);
			if (roomEntityList.empty())
			{
				response->set_errorcode(EL10nCode_NotDsServer);
				LoggerPrint::Log(channel, ELogLevel_Debug, "not ds Server");
			}
			else
			{
				roomEntity = roomEntityList.front()->GetSelf<RoomEntityHelper>();
			}
			
		}

		// req token
		if (roomEntity)
		{
			ServerProxyHelper::Ptr proxyHelper = dnServer->GetServerProxy();

			// wait data parse
			bool success = co_await proxyHelper->AddMsg(EMMsgDeal::Req, request, response, roomEntity->GetChannel());
		
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
