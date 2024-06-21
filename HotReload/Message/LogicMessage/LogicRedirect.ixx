module;
#include <coroutine>
#include <cstdint>
#include <list>
#include <string>

#include "StdMacro.h"
export module LogicMessage:LogicRedirect;

import DNTask;
import FuncHelper;
import LogicServerHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

namespace LogicMessage
{
	export void Exe_RetAccountReplace(SocketChannelPtr channel, uint32_t msgId,  string binMsg)
	{
		S2C_RetAccountReplace request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		LogicServerHelper* dnServer = GetLogicServer();
		ClientEntityManagerHelper* entityMan = dnServer->GetClientEntityManager();

		ClientEntity* entity = entityMan->GetEntity(request.account_id());
		if (!entity)
		{
			DNPrint(0, LoggerLevel::Debug, "Client Entity Kick Not Exist !");
			return;
		}

		RoomEntityManagerHelper* roomEntityMan = dnServer->GetRoomEntityManager();
		RoomEntity* roomEntity = roomEntityMan->GetEntity(entity->RecordRoomId());

		// cache
		if (roomEntity)
		{
			string binData = binMsg;

			MessagePackAndSend(0, MsgDeal::Ret, request.GetDescriptor()->full_name().c_str(), binData, roomEntity->GetSock());
		}
		else
		{
			DNPrint(0, LoggerLevel::Debug, "Client Entity Kick Server Not Exist !");
		}

		// close entity save data
		entityMan->RemoveEntity(entity->ID());
	}

	// client request
	export DNTaskVoid Msg_ReqClientLogin(SocketChannelPtr channel, uint32_t msgId,  string binMsg)
	{
		C2S_ReqAuthToken request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}
		S2C_ResAuthToken response;

		LogicServerHelper* dnServer = GetLogicServer();
		ClientEntityManagerHelper* entityMan = dnServer->GetClientEntityManager();

		ClientEntity* entity = entityMan->AddEntity(request.account_id());
		if (entity)
		{
			DNPrint(0, LoggerLevel::Debug, "AddEntity Client!");

			// msg will destroy. MessageHandle not will waiting.
			co_await entityMan->LoadEntityData(entity, nullptr, nullptr);

			if (!entity->HasFlag(ClientEntityFlag::DBInited))
			{
				DNPrint(0, LoggerLevel::Debug, "AddEntity Client but not from db!");
			}
		}
		else
		{
			DNPrint(0, LoggerLevel::Debug, "AddEntity Exist Client!");
			entity = entityMan->GetEntity(request.account_id());
		}

		RoomEntityManagerHelper* roomEntityMan = dnServer->GetRoomEntityManager();
		RoomEntity* roomEntity = nullptr;

		// cache
		if (uint32_t roomId = entity->RecordRoomId())
		{
			roomEntity = roomEntityMan->GetEntity(roomId);
		}

		//pool
		if (!roomEntity)
		{
			uint32_t mapId = 0;
			// from db
			if(entity->GetDbEntity()->has_map_info())
			{
				MapPointRecord* mapRecord = entity->GetDbEntity()->mutable_map_info();
				*mapRecord->mutable_cur_point() = *mapRecord->mutable_last_point();

				mapId = mapRecord->cur_point().map_id();
			}
			// new player use default 1
			else
			{
				mapId++;

				MapPointRecord* mapRecord = entity->GetDbEntity()->mutable_map_info();
				mapRecord->mutable_cur_point()->set_map_id(mapId);
			}

			list<RoomEntity*> roomEntityList = roomEntityMan->GetEntitysByMapId(mapId);
			if (roomEntityList.empty())
			{
				response.set_state_code(5);
				DNPrint(0, LoggerLevel::Debug, "not ds Server");
			}
			else
			{
				roomEntity = roomEntityList.front();
			}
			
		}

		string binData;

		// req token
		if (roomEntity)
		{
			binData = binMsg;

			
			auto taskGen = [](Message* msg) -> DNTask<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);

			DNServerProxyHelper* server = dnServer->GetSSock();
			uint32_t msgId = server->GetMsgId();

			// wait data parse
			server->AddMsg(msgId, &dataChannel, 8000);

			MessagePackAndSend(msgId, MsgDeal::Req, request.GetDescriptor()->full_name().c_str(), binData, roomEntity->GetSock());

			co_await dataChannel;

			if (dataChannel.HasFlag(DNTaskFlag::Timeout))
			{
				DNPrint(0, LoggerLevel::Debug, "requst timeout! ");
				response.set_state_code(6);
			}
			else
			{
				entity->RecordRoomId() = roomEntity->ID();
				//combin
				response.set_server_ip(roomEntity->ServerIp());
				response.set_server_port(roomEntity->ServerPort());
			}

		}

		DNPrint(0, LoggerLevel::Debug, "ds:%s", response.DebugString().c_str());

		// pack data
		response.SerializeToString(&binData);
		MessagePackAndSend(msgId, MsgDeal::Res, nullptr, binData, channel);

		co_return;
	}

}
