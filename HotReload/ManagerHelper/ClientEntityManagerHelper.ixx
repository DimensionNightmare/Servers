module;
#include "StdMacro.h"
export module ClientEntityManagerHelper;

import ClientEntityHelper;
import ClientEntityManager;
import Logger;
import DNTask;
import FuncHelper;
import StrUtils;
import ThirdParty.PbGen;
import DNServer;

export class ClientEntityManagerHelper : public ClientEntityManager
{

private:

	ClientEntityManagerHelper() = delete;
public:

	ClientEntity* AddEntity(uint32_t entityId)
	{
		if (!mEntityMap.contains(entityId))
		{
			std::unique_lock<std::shared_mutex> ulock(oMapMutex);
			mEntityMap.emplace(std::piecewise_construct,
				std::forward_as_tuple(entityId),
				std::forward_as_tuple(entityId));

			ClientEntity* entity = &mEntityMap[entityId];

			return entity;
		}

		return nullptr;
	}

	bool RemoveEntity(uint32_t entityId)
	{

		if (mEntityMap.contains(entityId))
		{
			std::unique_lock<std::shared_mutex> ulock(oMapMutex);

			DNPrint(ELogLevel_Debug, "destory client entity");
			mEntityMap.erase(entityId);

			return true;
		}

		return false;
	}

	ClientEntity* GetEntity(uint32_t entityId)
	{
		std::shared_lock<std::shared_mutex> lock(oMapMutex);
		if (mEntityMap.contains(entityId))
		{
			return &mEntityMap[entityId];
		}
		// allow return empty
		return nullptr;
	}

	DNTaskVoid LoadEntityData(ClientEntity* entity, d2L_ReqLoadEntityData* inRequest, L2d_ResLoadEntityData* inResponse)
	{
		if (!pSqlClient || pSqlClient->RegistType() != uint8_t(EMServerType::GateServer) || !pNoSqlProxy)
		{
			co_return;
		}

		DbModelPlayer* dbEntity = entity->GetDbEntity();

		if (entity->HasFlag(EMClientEntityFlag::DBInited) || entity->HasFlag(EMClientEntityFlag::DBIniting))
		{
			DNPrint(ELogLevel_Debug, "entity %u is DBIniting. return .", entity->ID());
			if (inResponse)
			{
				std::string* entity_data = inResponse->add_entity_data();
				dbEntity->SerializeToString(entity_data);
			}
			co_return;
		}

		std::string table_name = dbEntity->GetDescriptor()->full_name();
		uint32_t entityId = entity->ID();
		std::string keyName = std::format("{}_{}", table_name, entityId);

		// nosql
		std::string binData;
		if (auto res = pNoSqlProxy->get(keyName))
		{
			binData = res.value();
		}

		if (!binData.empty())
		{
			dbEntity->ParseFromString(binData);
			if (inResponse)
			{
				std::string* entity_data = inResponse->add_entity_data();
				*entity_data = binData;
			}

			entity->SetFlag(EMClientEntityFlag::DBInited);
			co_return;
		}

		entity->SetFlag(EMClientEntityFlag::DBIniting);
		// sql
		L2D_ReqLoadData request;

		if (inRequest)
		{
			request.set_table_name(inRequest->table_name());
			request.set_key_name(inRequest->key_name());
			request.set_entity_data(inRequest->entity_data());
			request.set_need_create(inRequest->need_create());
		}
		else
		{
			request.set_need_create(true);

			std::string* entity_data = request.mutable_entity_data();
			dbEntity->SerializeToString(entity_data);
		}

		request.set_limit(1);
		request.set_table_name(table_name);
		request.set_key_name(ClientEntity::SKeyName);

		request.SerializeToString(&binData);

		D2L_ResLoadData response;
		{
			auto taskGen = [](Message* msg) -> DNTask<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);

			uint32_t msgId = pSqlClient->GetMsgId();
			pSqlClient->AddMsg(msgId, &dataChannel, 9000);
			MessagePackAndSend(msgId, EMMsgDeal::Redir, request.GetDescriptor()->full_name().c_str(), binData, pSqlClient->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				response.set_state_code(10);
				DNPrint(ELogLevel_Debug, "requst timeout! ");
			}
		}

		if (int code = response.state_code())
		{
			entity->ClearFlag(EMClientEntityFlag::DBIniting);

			binData = request.entity_data();
			BytesToHexString(binData);
			mDbFailure[entityId] = binData;
			DNPrint(ELogLevel_Debug, "Load Db Entity Error id = %u, state_code = %d! ", entityId, code);
			co_return;
		}

		if (int lenth = response.entity_data_size(); lenth == 1)
		{
			const std::string entityData = response.entity_data(0);
			dbEntity->ParseFromString(entityData);
			entity->SetFlag(EMClientEntityFlag::DBInited);

			pNoSqlProxy->set(keyName, entityData);
		}
		else
		{
			DNPrint(ELogLevel_Debug, "Load Db Entity mutiply data!");
			response.clear_entity_data();
		}

		entity->ClearFlag(EMClientEntityFlag::DBIniting);

		if (inResponse)
		{
			inResponse->set_state_code(response.state_code());
			for (int i = 0; i < response.entity_data_size(); i++)
			{
				std::string* bytes = inResponse->add_entity_data();
				*bytes = response.entity_data(i);
			}
		}

		co_return;
	}

	void ClearNosqlProxy() { pNoSqlProxy = nullptr; }
};
