module;
export module ClientEntityManagerHelper;

import ClientEntityManager;
import StrUtils;
import FuncHelper;
import MdbProxy;
import ClientEntityHelper;
import DllUtils;

#define FUNCPLACE(class, func) &class::func, #class"_"#func

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
			TickMainSpaceDll(this, FUNCPLACE(ClientEntityManager,AddEntity), entityId);

			ClientEntity::Ptr entity = mEntityMap[entityId];
			
			return entity;
		}

		return nullptr;
	}

	bool RemoveEntity(uint64_t entityId)
	{

		if (mEntityMap.contains(entityId))
		{
			GetLogger()->Record(ELogLevel_Debug, "destory client entity");
			ClientEntity::Ptr& entity = mEntityMap[entityId];
			entity->Dispose();

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

	DNTaskVoid LoadEntityData(const ClientEntityHelper::Ptr& entity, GMsg::d2L_ReqLoadEntityData* inRequest, GMsg::L2d_ResLoadEntityData* inResponse)
	{
		if (!pSqlClient || pSqlClient->RegistType() != static_cast<uint8_t>(EMServerType::GateServer))
		{
			co_return;
		}

		
		if (entity->HasFlag(EMClientEntityFlag::DBInited))
		{
			if(inResponse)
			{
				std::string* entity_data = inResponse->add_entity_data();
				GDb::PlayerPtr dbEntity = entity->GetDbEntity();
				dbEntity->SerializeToString(entity_data);
			}
			co_return;
		}
		else if(entity->HasFlag(EMClientEntityFlag::DBIniting))
		{
			GetLogger()->Record(ELogLevel_Debug, "entity {} is DBIniting. return .", entity->ID());
			if (inResponse)
			{
				inResponse->set_error_code(EL10nCode_DBIniting);
			}
			co_return;
		}
		
		entity->SetFlag(EMClientEntityFlag::DBIniting);

		std::string binData;

		std::string table_name = GDb::Player::GetDescriptor()->full_name();
		uint64_t entityId = entity->ID();
		std::string keyName = std::format("{}_{}", table_name, entityId);

		MdbProxy::Ptr dbProxy = GetOwner()->GetComponent<MdbProxy>(EMComponentType::MdbProxy);
		if(auto connection = dbProxy->GetConnection())
		{
			// nosql
			if (auto res = connection->get(keyName))
			{
				binData = res.value();
			}

			if (!binData.empty())
			{
				entity->SetDbEntity(binData);

				if (inResponse)
				{
					std::string* entity_data = inResponse->add_entity_data();
					*entity_data = binData;
				}

				entity->SetFlag(EMClientEntityFlag::DBInited);
				co_return;
			}
		}

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
			// request.set_need_create(true);

			// std::string* entity_data = request.mutable_entity_data();
			// dbEntity->SerializeToString(entity_data);
			request.set_limit(1);
			request.set_table_name(table_name);
			request.set_key_name(ClientEntity::SKeyName);
			
			GDb::Player temp;
			temp.set_account_id(entityId);
			temp.SerializeToString(request.mutable_entity_data());
		}


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
		
		entity->ClearFlag(EMClientEntityFlag::DBIniting);

		if (response.error_code() != EL10nCode_None)
		{

			// binData = request.entity_data();
			// BytesToHexString(binData);
			// mDbFailure[entityId] = binData;
			GetLogger()->Record(ELogLevel_Debug, "Load Db Entity Error id = {}, error_code = {}! ", entityId, static_cast<int>(response.error_code()));
			co_return;
		}

		entity->SetFlag(EMClientEntityFlag::DBInited);

		int lenth = response.entity_data_size();
		if (lenth == 1)
		{
			const std::string& entityData = response.entity_data(0);
			entity->SetDbEntity(entityData);
			
			if(auto connection = dbProxy->GetConnection())
			{
				connection->set(keyName, entityData);
			}

		}
		else if(lenth > 1)
		{
			GetLogger()->Record(ELogLevel_Debug, "Load Db Entity mutiply data!");
		}


		if (inResponse)
		{
			inResponse->set_error_code(response.error_code());
			for (int i = 0; i < lenth; i++)
			{
				std::string* bytes = inResponse->add_entity_data();
				*bytes = response.entity_data(i);
			}
		}

		co_return;
	}
};
