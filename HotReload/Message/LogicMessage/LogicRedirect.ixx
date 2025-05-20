module;
export module LogicMessage:LogicRedirect;

import DNTask;
import FuncHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ClientEntityManagerHelper;
import std.compat;
import RoomEntity;
import LogicServerHelper;
import ECSW;

namespace LogicMessage
{
	export void Exe_RetAccountReplace(const DNSocketChannel::Ptr& channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::S2C_RetAccountReplace request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		LogicServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::DNServer);
		ClientEntityManagerHelper::Ptr entityMan = dnServer->GetClientEntityManager();

		ClientEntity::Ptr entity = entityMan->GetEntity(request.account_id());
		if (!entity)
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "Client Entity Kick Not Exist !");
			return;
		}

		RoomEntityManagerHelper::Ptr roomEntityMan = dnServer->GetRoomEntityManager();
		RoomEntity::Ptr roomEntity = roomEntityMan->GetEntity(entity->RecordRoomId());

		// cache
		if (roomEntity)
		{
			std::string binData = binMsg;

			MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, roomEntity->GetChannel());
		}
		else
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "Client Entity Kick Server Not Exist !");
		}

		// close entity save data
		entityMan->RemoveEntity(entity->ID());
	}

	// client request
	export DNTaskVoid Msg_ReqClientLogin(const DNSocketChannel::Ptr& channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::C2S_ReqAuthToken request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}
		GMsg::S2C_ResAuthToken response;

		FinalExecute final([&response, msgId, channel](){
			std::string binData;
			response.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Res, binData, channel);
		});

		LogicServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::DNServer);
		ClientEntityManagerHelper::Ptr entityMan = dnServer->GetClientEntityManager();

		ClientEntity::Ptr entity = entityMan->AddEntity(request.account_id());
		if (entity)
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "AddEntity Client!");

			// msg will destroy. MessageHandle not will waiting.
			co_await entityMan->LoadEntityData(entity, nullptr, nullptr);

			if (!entity->HasFlag(EMClientEntityFlag::DBInited))
			{
				dnServer->GetLogger()->Record(ELogLevel_Debug, "AddEntity Client but not from db!");
			}
		}
		else
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "AddEntity Exist Client!");
			entity = entityMan->GetEntity(request.account_id());
		}

		RoomEntityManagerHelper::Ptr roomEntityMan = dnServer->GetRoomEntityManager();
		RoomEntity::Ptr roomEntity = nullptr;

		// cache
		if (uint64_t roomId = entity->RecordRoomId())
		{
			roomEntity = roomEntityMan->GetEntity(roomId);
		}

		//pool
		if (!roomEntity)
		{
			uint64_t mapId = 0;
			// from db
			if(entity->GetDbEntity()->has_map_info())
			{
				GDef_MapPointRecord* mapRecord = entity->GetDbEntity()->mutable_map_info();
				*mapRecord->mutable_cur_point() = *mapRecord->mutable_last_point();

				mapId = mapRecord->cur_point().map_id();
			}
			// new player use default 1
			else
			{
				mapId++;

				GDef_MapPointRecord* mapRecord = entity->GetDbEntity()->mutable_map_info();
				mapRecord->mutable_cur_point()->set_map_id(mapId);
			}

			std::list<RoomEntity::Ptr> roomEntityList = roomEntityMan->GetEntitysByMapId(mapId);
			if (roomEntityList.empty())
			{
				response.set_state_code(5);
				dnServer->GetLogger()->Record(ELogLevel_Debug, "not ds Server");
			}
			else
			{
				roomEntity = roomEntityList.front();
			}
			
		}

		// req token
		if (roomEntity)
		{
			auto taskGen = [](Message* msg) -> DNTask<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);

			DNServerProxyHelper::Ptr server = dnServer->GetServerProxy();
			uint32_t msgId = server->GetMsgId();

			// wait data parse
			server->AddMsg(msgId, &dataChannel, 8000);

			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name(), binMsg, roomEntity->GetChannel());

			co_await dataChannel;

			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				dnServer->GetLogger()->Record(ELogLevel_Debug, "requst timeout! ");
				response.set_state_code(6);
			}
			else
			{
				entity->SetRecordRoomId(roomEntity->ID());
				//combin
				response.set_server_ip(roomEntity->ServerIp());
				response.set_server_port(roomEntity->ServerPort());
			}

		}

		dnServer->GetLogger()->Record(ELogLevel_Debug, "ds:{}", response.DebugString());

		co_return;
	}

}
