module;
export module LogicServerMessage:LogicRedirect;

import LogicServerHelper;

import ThirdParty.Libhv;
import FuncHelper;

namespace LogicServerMessage
{
	export void Exe_RetAccountReplace(DNSocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::S2C_RetAccountReplace request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		LogicServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::DNServer);
		ClientEntityManagerHelper::CVPtr entityMan = dnServer->GetClientEntityManager();

		ClientEntityHelper::CVPtr entity = entityMan->GetEntity(request.account_id());
		if (!entity)
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "Client Entity Kick Not Exist !");
			return;
		}

		RoomEntityManagerHelper::CVPtr roomEntityMan = dnServer->GetRoomEntityManager();
		RoomEntityHelper::CVPtr roomEntity = roomEntityMan->GetEntity(entity->RecordRoomId());

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
	export DNTaskVoid Msg_ReqClientLogin(DNSocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
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

		LogicServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::DNServer);
		ClientEntityManagerHelper::CVPtr entityMan = dnServer->GetClientEntityManager();

		ClientEntityHelper::Ptr entity = entityMan->AddEntity(request.account_id());
		if (entity)
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "AddEntity Client!");

			// msg will destroy. MessageHandle not will waiting.
			co_await entityMan->LoadEntity(entity->GetSelf<ClientEntityHelper>(), nullptr, nullptr);

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

#if 1
		RoomEntityManagerHelper::CVPtr roomEntityMan = dnServer->GetRoomEntityManager();
		RoomEntityHelper::Ptr roomEntity = nullptr;

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
				response.set_error_code(EL10nCode_NotDsServer);
				dnServer->GetLogger()->Record(ELogLevel_Debug, "not ds Server");
			}
			else
			{
				roomEntity = roomEntityList.front()->GetSelf<RoomEntityHelper>();
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

			DNServerProxyHelper::CVPtr server = dnServer->GetServerProxy();
			uint32_t msgId = server->GetMsgId();

			// wait data parse
			server->AddMsg(msgId, &dataChannel, 8000);

			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name(), binMsg, roomEntity->GetChannel());

			co_await dataChannel;

			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				response.set_error_code(EL10nCode_ReqRegistTimeout);
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
#endif

		co_return;
	}

}
