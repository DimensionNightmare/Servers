export module LogicServerMessage:LogicRedirect;

import LogicServerHelper;
import std;
import ThirdParty.Libhv;
import FuncHelper;
import ThirdParty.PbGen;
import Logger;

export namespace LogicServerMessage
{
	void Exe_RetAccountReplace(SocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::S2C_RetAccountReplace request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		LogicServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);
		ClientEntityManagerHelper::CVPtr entityMan = dnServer->GetClientEntityManager();

		ClientEntityHelper::CVPtr entity = entityMan->GetEntity(request.accountid());
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
			std::string binData = binMsg;

			MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, roomEntity->GetChannel());
		}
		else
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "Client Entity Kick Server Not Exist !");
		}

		// close entity save data
		entityMan->RemoveEntity(entity->ID());
	}

	// client request
	TaskVoid Msg_ReqClientLogin(SocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
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

		LogicServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);
		ClientEntityManagerHelper::CVPtr entityMan = dnServer->GetClientEntityManager();

		ClientEntityHelper::Ptr entity = entityMan->AddEntity(request.accountid());
		if (entity)
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "AddEntity Client!");

			// msg will destroy. MessageHandle not will waiting.
			co_await entityMan->LoadEntity(entity->GetSelf<ClientEntityHelper>(), nullptr, nullptr);

			if (!entity->HasFlag(EMClientEntityFlag::DBInited))
			{
				LoggerPrint::Log(channel, ELogLevel_Debug, "AddEntity Client but not from db!");
			}
		}
		else
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "AddEntity Exist Client!");
			entity = entityMan->GetEntity(request.accountid());
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
				response.set_errorcode(EL10nCode_NotDsServer);
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
			auto taskGen = [](Message* msg) -> Task<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);

			ServerProxyHelper::CVPtr server = dnServer->GetServerProxy();
			uint32_t msgId = server->GetMsgId();

			// wait data parse
			server->AddMsg(msgId, &dataChannel, 8000);

			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name(), binMsg, roomEntity->GetChannel());

			co_await dataChannel;

			if (dataChannel.HasFlag(EMTaskFlag::Timeout))
			{
				response.set_errorcode(EL10nCode_ReqRegistTimeout);
			}
			else
			{
				entity->SetRecordRoomId(roomEntity->ID());
				//combin
				response.set_serverip(roomEntity->ServerIp());
				response.set_serverport(roomEntity->ServerPort());
			}

		}

		LoggerPrint::Log(channel, ELogLevel_Debug, "ds:{}", response.DebugString());
#endif

		co_return;
	}

}
