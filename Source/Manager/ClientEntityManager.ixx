module;
export module ClientEntityManager;

import ClientEntity;
import DNServer;
import EntityManager;
import Logger;
import DNClientProxy;
import DNTask;
import FuncHelper;
import StrUtils;
import ThirdParty.PbGen;
import ThirdParty.RedisPP;

/// @brief manager client proxys
export class ClientEntityManager : public EntityManager<ClientEntity>
{
	
public:
	ClientEntityManager() = default;

	virtual ~ClientEntityManager()
	{
		CheckSaveEntity(true);
	}

	/// @brief start timer manager
	virtual bool Init() override
	{
		return EntityManager::Init();
	}

	/// @brief redisConnection pointer save
	void InitSqlConn(const std::shared_ptr<sw::redis::Redis>& redisConn)
	{
		pNoSqlProxy = redisConn;
	}

	/// @brief server self pointer save. mean connected father node success.
	void InitSqlConn(DNClientProxy* sockClient)
	{
		pSqlClient = sockClient;
	}

	virtual void TickMainFrame() override
	{
		CheckSaveEntity();
	}

public: // dll override
	/// @brief save entity data to database. this is task.
	DNTaskVoid SaveEntity(ClientEntity& entity, bool offline = false)
	{
		uint32_t entityId = entity.ID();

		GDb::Player dbEntity = *entity.GetDbEntity();

		// change maprecord
		if(offline)
		{
			GDef_MapPointRecord* mapInfo = dbEntity.mutable_map_info();
			GDef_MapPoint* cur_point = mapInfo->mutable_cur_point();
			GDef_Vector3* property_location = dbEntity.mutable_property_entity()->mutable_location();
			*cur_point->mutable_point() = *property_location;
			property_location->Clear();

			GDef_MapPoint* last_point = mapInfo->mutable_last_point();
			*last_point = *cur_point;
			cur_point->Clear();
		}

		std::string entity_data;
		dbEntity.SerializeToString(&entity_data);

		// sql
		GMsg::L2D_ReqSaveData request;
		std::string table_name = dbEntity.GetDescriptor()->full_name();
		request.set_table_name(table_name);
		request.set_key_name(ClientEntity::SKeyName);
		request.set_entity_data(entity_data);

		GMsg::D2L_ResSaveData response;

		{
			auto taskGen = [](Message* msg) -> DNTask<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);

			uint32_t msgId = pSqlClient->GetMsgId();
			pSqlClient->AddMsg(msgId, &dataChannel, 9000);

			std::string binData;
			request.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Redir, request.GetDescriptor()->full_name(), binData, pSqlClient->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				response.set_state_code(10);
				LoggerPrint()(ELogLevel_Debug, "requst timeout! ");
			}
		}

		if (int code = response.state_code())
		{
			BytesToHexString(entity_data);
			mDbFailure[entityId] = entity_data;
			LoggerPrint()(ELogLevel_Debug, "Save Db Entity Error id = {}, state_code = {}! ", entityId, code);
			co_return;
		}

		// nosql
		std::string keyName = std::format("{}_{}", table_name, entityId);
		pNoSqlProxy->set(keyName, entity_data);

		mDbFailure.erase(entityId);
		co_return;
	}

	/// @brief save entity data list slow.
	void CheckSaveEntity(bool shutdown = false)
	{

		std::function<void(ClientEntity&, bool)> dealFunc = nullptr;

		if (!pSqlClient || pSqlClient->RegistType() != uint8_t(EMServerType::GateServer) || !pNoSqlProxy)
		{
			dealFunc = [this](ClientEntity& entity, bool offline)
				{
					std::string binData;
					uint32_t entityId = entity.ID();
					if (!entity.GetDbEntity())
					{
						LoggerPrint()(ELogLevel_Debug, "SaveEntity not pb Data:{}", entityId);
						return;
					}

					entity.GetDbEntity()->SerializeToString(&binData);

					BytesToHexString(binData);
					mDbFailure[entityId] = binData;
				};
		}
		else
		{
			dealFunc = std::bind(&ClientEntityManager::SaveEntity, this, std::placeholders::_1, std::placeholders::_2);
		}

		for (auto& [ID, entity] : mEntityMap)
		{
			if (!entity.GetDbEntity())
			{
				LoggerPrint()(ELogLevel_Debug, "SaveEntity not pb Data:{}", ID);
				continue;
			}

			if(shutdown)
			{
				dealFunc(entity, shutdown);
				continue;
			}

			if (entity.HasFlag(EMClientEntityFlag::DBModify))
			{
				entity.ClearFlag(EMClientEntityFlag::DBModify);

				dealFunc(entity, shutdown);
			}
		}
	}

protected: // dll proxy
	std::shared_ptr<sw::redis::Redis> pNoSqlProxy;
	DNClientProxy* pSqlClient;

	/// @brief if save error. bin data will record to this.
	std::unordered_map<uint32_t, std::string> mDbFailure;
	
};
