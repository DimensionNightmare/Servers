export module ClientEntityManagerHelper;

import ClientEntityManager;
import StrUtils;
import FuncHelper;
import MdbProxyHelper;
import ClientEntityHelper;
import Task;
import ThirdParty.PbGen;
import ClientProxyHelper;

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
	using CVPtr = const Ptr&;

	ClientEntityHelper::Ptr AddEntity(uint64_t entityId)
	{
		if (!mEntityMap.contains(entityId))
		{
			return pAddEntity(entityId)->GetSelf<ClientEntityHelper>();
		}

		return nullptr;
	}

	ClientEntityHelper::Ptr GetEntity(uint64_t entityId)
	{
		std::shared_lock lock(oMapMutex);
		if (mEntityMap.contains(entityId))
		{
			return mEntityMap[entityId]->GetSelf<ClientEntityHelper>();
		}
		// allow return empty
		return nullptr;
	}

	TaskVoid LoadEntity(ClientEntityHelper::CVPtr entity, GMsg::d2L_ReqLoadEntityData* inRequest, GMsg::L2d_ResLoadEntityData* inResponse)
	{
		ClientProxyHelper::CVPtr sqlClient = pSqlClient->GetSelf<ClientProxyHelper>();

		if (!sqlClient || sqlClient->RegistType() != static_cast<uint8_t>(EMServerType::GateServer))
		{
			co_return;
		}

		
		if (entity->HasFlag(EMClientEntityFlag::DBInited))
		{
			if(inResponse)
			{
				std::string* entity_data = inResponse->add_entity_data();
				GDb::Player* dbEntity = entity->GetDbEntity();
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

		MdbProxyHelper::CVPtr dbProxy = GetOwner()->GetComponent<MdbProxyHelper>(EMComponentType::MdbProxy);
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

		GDb::Player* dbEntity = entity->GetDbEntity();

		// only query db data
		if (inRequest)
		{
			request.set_table_name(inRequest->table_name());
			request.set_key_name(inRequest->key_name());
			request.set_entity_data(inRequest->entity_data());
			request.set_need_create(inRequest->need_create());
		}
		// this mean new Entity branch
		else
		{
			request.set_need_create(true);

			request.set_limit(1);
			request.set_table_name(table_name);
			request.set_key_name(ClientEntity::SKeyName);
			
			dbEntity->SerializeToString(request.mutable_entity_data());
		}


		request.SerializeToString(&binData);

		GMsg::D2L_ResLoadData response;
		{
			auto taskGen = [](Message* msg) -> Task<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);

			uint32_t msgId = sqlClient->GetMsgId();
			sqlClient->AddMsg(msgId, &dataChannel, 9000);
			MessagePackAndSend(msgId, EMMsgDeal::Redir, request.GetDescriptor()->full_name(), binData, sqlClient->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMTaskFlag::Timeout))
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

	/// @brief save entity data to database. this is task.
	TaskVoid SaveEntity(ClientEntityHelper::CVPtr entity, bool offline = false)
	{
		uint64_t entityId = entity->ID();

		if(!entity->HasFlag(EMClientEntityFlag::DBInited))
		{
			co_return;
		}
		
		GDb::Player* dbEntity = entity->GetDbEntity();

		// change maprecord
		if(offline)
		{
			GDef_MapPointRecord* mapInfo = dbEntity->mutable_map_info();
			GDef_MapPoint* cur_point = mapInfo->mutable_cur_point();
			GDef_Vector3* property_location = dbEntity->mutable_property_entity()->mutable_location();
			*cur_point->mutable_point() = *property_location;
			property_location->Clear();

			GDef_MapPoint* last_point = mapInfo->mutable_last_point();
			*last_point = *cur_point;
			cur_point->Clear();
		}

		std::string entity_data;
		dbEntity->SerializeToString(&entity_data);

		// sql
		GMsg::L2D_ReqSaveData request;
		std::string table_name = dbEntity->GetDescriptor()->full_name();
		request.set_table_name(table_name);
		request.set_key_name(ClientEntity::SKeyName);
		request.set_entity_data(entity_data);

		GMsg::D2L_ResSaveData response;

		{
			ClientProxyHelper::CVPtr sqlClient = pSqlClient->GetSelf<ClientProxyHelper>();

			auto taskGen = [](Message* msg) -> Task<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);

			uint32_t msgId = sqlClient->GetMsgId();
			sqlClient->AddMsg(msgId, &dataChannel, 9000);

			std::string binData;
			request.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Redir, request.GetDescriptor()->full_name(), binData, sqlClient->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMTaskFlag::Timeout))
			{
				response.set_error_code(EL10nCode_CRdbReqTimeout);
			}
		}

		if (response.error_code() != EL10nCode_None)
		{
			BytesToHexString(entity_data);
			mDbFailure[entityId] = entity_data;
			GetLogger()->Record(ELogLevel_Debug, "Save Db Entity Error id = {}, error_code = {}! ", entityId, static_cast<int>(response.error_code()));
			co_return;
		}

		// nosql
		MdbProxyHelper::CVPtr dbProxy = GetOwner()->GetComponent<MdbProxyHelper>(EMComponentType::MdbProxy);
		if(auto connection = dbProxy->GetConnection())
		{
			std::string keyName = std::format("{}_{}", table_name, entityId);
			connection->set(keyName, entity_data);
		}

		mDbFailure.erase(entityId);
		co_return;
	}

	/// @brief save entity data list slow.
	void CheckSaveEntity(bool shutdown = false)
	{

		std::function<void(ClientEntityHelper::CVPtr, bool)> dealFunc = nullptr;
		
		ClientProxyHelper::CVPtr sqlClient = pSqlClient->GetSelf<ClientProxyHelper>();

		if (!sqlClient || sqlClient->RegistType() != uint8_t(EMServerType::GateServer))
		{
			dealFunc = [this](ClientEntityHelper::CVPtr entity, bool offline)
				{
					std::string binData;
					uint64_t entityId = entity->ID();
					if (!entity->GetDbEntity())
					{
						GetLogger()->Record(ELogLevel_Debug, "SaveEntity not pb Data:{}", entityId);
						return;
					}

					entity->GetDbEntity()->SerializeToString(&binData);

					BytesToHexString(binData);
					mDbFailure[entityId] = binData;
				};
		}
		else
		{
			dealFunc = std::bind(&ClientEntityManagerHelper::SaveEntity, this, std::placeholders::_1, std::placeholders::_2);
		}

		for (auto& [ID, entity] : mEntityMap)
		{
			if (!entity->GetDbEntity())
			{
				GetLogger()->Record(ELogLevel_Debug, "SaveEntity not pb Data:{}", ID);
				continue;
			}

			if(shutdown)
			{
				dealFunc(entity->GetSelf<ClientEntityHelper>(), shutdown);
				continue;
			}

			if (entity->HasFlag(EMClientEntityFlag::DBModify))
			{
				entity->ClearFlag(EMClientEntityFlag::DBModify);

				dealFunc(entity->GetSelf<ClientEntityHelper>(), shutdown);
			}
		}
	}
	
	/// @brief server self pointer save. mean connected father node success.
	void InitSqlConn(ClientProxy::CVPtr sockClient)
	{
		pSqlClient = sockClient;
	}

};
