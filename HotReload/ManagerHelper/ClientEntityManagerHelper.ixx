module;
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
	~ClientEntityManagerHelper() = default;

	ClientEntityManagerHelper(const ClientEntityManagerHelper&) = delete;
	void operator=(const ClientEntityManagerHelper&) = delete;

	ClientEntityManagerHelper(ClientEntityManagerHelper&&) = delete;
	ClientEntityManagerHelper& operator=(ClientEntityManagerHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<ClientEntityManagerHelper>;

	ClientEntity::Ptr AddEntity(uint64_t entityId)
	{
		if (!mEntityMap.contains(entityId))
		{
			ClientEntity::Ptr entity = std::shared_ptr<ClientEntity>(new ClientEntity(GetOwner()->GetWorldW()));
			

			std::unique_lock<std::shared_mutex> ulock(oMapMutex);
			mEntityMap[entityId] = entity;
			return entity;
		}

		return nullptr;
	}

	bool RemoveEntity(uint64_t entityId)
	{

		if (mEntityMap.contains(entityId))
		{
			SPidLogger.Record(ELogLevel_Debug, "destory client entity");


			std::unique_lock<std::shared_mutex> ulock(oMapMutex);
			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

	ClientEntity::Ptr GetEntity(uint64_t entityId)
	{
		std::shared_lock<std::shared_mutex> lock(oMapMutex);
		if (mEntityMap.contains(entityId))
		{
			return mEntityMap[entityId];
		}
		// allow return empty
		return nullptr;
	}

	DNTaskVoid LoadEntityData(ClientEntity::Ptr entity, GMsg::d2L_ReqLoadEntityData* inRequest, GMsg::L2d_ResLoadEntityData* inResponse)
	{
		if (!pSqlClient || !pNoSqlProxy || pSqlClient->RegistType() != static_cast<uint8_t>(EMServerType::GateServer))
		{
			co_return;
		}

		GDb::PlayerPtr dbEntity = entity->GetDbEntity();

		if (entity->HasFlag(EMClientEntityFlag::DBInited) || entity->HasFlag(EMClientEntityFlag::DBIniting))
		{
			SPidLogger.Record(ELogLevel_Debug, "entity {} is DBIniting. return .", entity->ID());
			if (inResponse)
			{
				std::string* entity_data = inResponse->add_entity_data();
				dbEntity->SerializeToString(entity_data);
			}
			co_return;
		}

		std::string table_name = dbEntity->GetDescriptor()->full_name();
		uint64_t entityId = entity->ID();
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
		GMsg::L2D_ReqLoadData request;

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

		GMsg::D2L_ResLoadData response;
		{
			auto taskGen = [](Message* msg) -> DNTask<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);

			uint32_t msgId = pSqlClient->GetMsgId();
			pSqlClient->AddMsg(msgId, &dataChannel, 9000);
			MessagePackAndSend(msgId, EMMsgDeal::Redir, request.GetDescriptor()->full_name(), binData, pSqlClient->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				response.set_error_code(EL10nCode_CRdbReqTimeout);
			}
		}

		if (response.error_code() != EL10nCode_None)
		{
			entity->ClearFlag(EMClientEntityFlag::DBIniting);

			binData = request.entity_data();
			BytesToHexString(binData);
			mDbFailure[entityId] = binData;
			SPidLogger.Record(ELogLevel_Debug, "Load Db Entity Error id = {}, error_code = {}! ", entityId, static_cast<int>(response.error_code()));
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
			SPidLogger.Record(ELogLevel_Debug, "Load Db Entity mutiply data!");
			response.clear_entity_data();
		}

		entity->ClearFlag(EMClientEntityFlag::DBIniting);

		if (inResponse)
		{
			inResponse->set_error_code(response.error_code());
			for (int i = 0; i < response.entity_data_size(); i++)
			{
				std::string* bytes = inResponse->add_entity_data();
				*bytes = response.entity_data(i);
			}
		}

		co_return;
	}
};
