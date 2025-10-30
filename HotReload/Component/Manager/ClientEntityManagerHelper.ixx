export module ClientEntityManagerHelper;

import ClientEntityManager;
import StrUtils;
import FuncHelper;
import MdbProxyHelper;
import ClientEntityHelper;
import Task;
import ClientProxyHelper;
import FuncUtils;
import Logger;

export class ClientEntityManagerHelper : public Helper<ClientEntityManagerHelper, ClientEntityManager>
{

private:

	ClientEntityManagerHelper() = delete;
	~ClientEntityManagerHelper() = default;

public:

	ClientEntityHelper::Ptr AddEntity(size_t entityId)
	{
		if (!mEntityMap.contains(entityId))
		{
			ClientEntity::Ptr entity = Base()->AddEntity(entityId);
			entity->GetDbEntity()->set_accountid(entityId);

			return entity->GetSelf<ClientEntityHelper>();
		}

		return nullptr;
	}

	ClientEntityHelper::Ptr GetEntity(size_t entityId)
	{
		std::shared_lock lock(oMapMutex);
		if (mEntityMap.contains(entityId))
		{
			return mEntityMap[entityId]->GetSelf<ClientEntityHelper>();
		}
		// allow return empty
		return nullptr;
	}

	TaskVoid LoadEntity(ClientEntityHelper::Ptr entity, GMsg::d2L_ReqLoadEntityData* inRequest, GMsg::L2d_ResLoadEntityData* inResponse)
	{
		ClientProxyHelper::Ptr clientProxy = GetOwner()->GetComponent<ClientProxyHelper>(EMComponentType::ClientProxy);

		if (!clientProxy || clientProxy->RegistType() != std::to_underlying(EMServerType::GateServer))
		{
			co_return;
		}

		
		if (entity->HasFlag(EMClientEntityFlag::DBInited))
		{
			if(inResponse)
			{
				std::string* entitydata = inResponse->add_entitydata();
				GDb::Player* dbEntity = entity->GetDbEntity();
				dbEntity->SerializeToString(entitydata);
			}
			co_return;
		}
		else if(entity->HasFlag(EMClientEntityFlag::DBIniting))
		{
			LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "entity {} is DBIniting. return .", entity->ID());
			if (inResponse)
			{
				inResponse->set_errorcode(EL10nCode_DBIniting);
			}
			co_return;
		}
		
		entity->SetFlag(EMClientEntityFlag::DBIniting);

		std::string binData;

		std::string tablename = GDb::Player::GetDescriptor()->full_name();
		size_t entityId = entity->ID();
		std::string keyName = std::format("{}_{}", tablename, entityId);

		MdbProxyHelper::Ptr dbProxy = GetOwner()->GetComponent<MdbProxyHelper>(EMComponentType::MdbProxy);
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
					std::string* entitydata = inResponse->add_entitydata();
					*entitydata = binData;
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
			request.set_tablename(inRequest->tablename());
			request.set_keynumber(inRequest->keynumber());
			request.set_entitydata(inRequest->entitydata());
			request.set_needcreate(inRequest->needcreate());
		}
		// this mean new Entity branch
		else
		{
			request.set_needcreate(true);

			request.set_limit(1);
			request.set_tablename(tablename);
			request.set_keynumber(GDb::Player::kAccountIdFieldNumber);
			
			dbEntity->SerializeToString(request.mutable_entitydata());
		}


		request.SerializeToString(&binData);

		GMsg::D2L_ResLoadData response;
		{
			auto taskGen = [](Message* msg) -> Task<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);

			uint32_t msgId = clientProxy->GetMsgId();
			clientProxy->AddMsg(msgId, &dataChannel, 9000);
			MessagePackAndSend(msgId, EMMsgDeal::Redir, request.GetDescriptor()->full_name(), binData, clientProxy->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMTaskFlag::Timeout))
			{
				response.set_errorcode(EL10nCode_CRdbReqTimeout);
			}
		}
		
		entity->ClearFlag(EMClientEntityFlag::DBIniting);

		if (response.errorcode() != EL10nCode_None)
		{

			// binData = request.entitydata();
			// BytesToHexString(binData);
			// mDbFailure[entityId] = binData;
			LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Load Db Entity Error id = {}, errorcode = {}! ", entityId, std::to_underlying(response.errorcode()));
			co_return;
		}

		entity->SetFlag(EMClientEntityFlag::DBInited);

		int lenth = response.entitydata_size();
		if (lenth == 1)
		{
			const std::string& entityData = response.entitydata(0);
			entity->SetDbEntity(entityData);
			
			if(auto connection = dbProxy->GetConnection())
			{
				connection->set(keyName, entityData);
			}

		}
		else if(lenth > 1)
		{
			LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Load Db Entity mutiply data!");
		}


		if (inResponse)
		{
			inResponse->set_errorcode(response.errorcode());
			for (int i = 0; i < lenth; i++)
			{
				std::string* bytes = inResponse->add_entitydata();
				*bytes = response.entitydata(i);
			}
		}

		co_return;
	}

	/// @brief save entity data to database. this is task.
	TaskVoid SaveEntity(ClientEntityHelper::Ptr entity, bool offline = false)
	{
		size_t entityId = entity->ID();

		if(!entity->HasFlag(EMClientEntityFlag::DBInited))
		{
			co_return;
		}
		
		GDb::Player* dbEntity = entity->GetDbEntity();

		// change maprecord
		if(offline)
		{
			GDef_MapPointRecord* mapInfo = dbEntity->mutable_mapinfo();
			GDef_MapPoint* curpoint = mapInfo->mutable_curpoint();
			GDef_Vector3* property_location = dbEntity->mutable_propertyentity()->mutable_location();
			*curpoint->mutable_point() = *property_location;
			property_location->Clear();

			GDef_MapPoint* lastpoint = mapInfo->mutable_lastpoint();
			*lastpoint = *curpoint;
			curpoint->Clear();
		}

		std::string entitydata;
		dbEntity->SerializeToString(&entitydata);

		// sql
		GMsg::L2D_ReqSaveData request;
		std::string tablename = dbEntity->GetDescriptor()->full_name();
		request.set_tablename(tablename);
		request.set_keynumber(GDb::Player::kAccountIdFieldNumber);
		request.set_entitydata(entitydata);

		GMsg::D2L_ResSaveData response;

		{
			ClientProxyHelper::Ptr clientProxy = GetOwner()->GetComponent<ClientProxyHelper>(EMComponentType::ClientProxy);

			auto taskGen = [](Message* msg) -> Task<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);

			uint32_t msgId = clientProxy->GetMsgId();
			clientProxy->AddMsg(msgId, &dataChannel, 9000);

			std::string binData;
			request.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Redir, request.GetDescriptor()->full_name(), binData, clientProxy->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMTaskFlag::Timeout))
			{
				response.set_errorcode(EL10nCode_CRdbReqTimeout);
			}
		}

		if (response.errorcode() != EL10nCode_None)
		{
			BytesToHexString(entitydata);
			mDbFailure[entityId] = entitydata;
			LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Save Db Entity Error id = {}, errorcode = {}! ", entityId, std::to_underlying(response.errorcode()));
			co_return;
		}

		// nosql
		MdbProxyHelper::Ptr dbProxy = GetOwner()->GetComponent<MdbProxyHelper>(EMComponentType::MdbProxy);
		if(auto connection = dbProxy->GetConnection())
		{
			std::string keyName = std::format("{}_{}", tablename, entityId);
			connection->set(keyName, entitydata);
		}

		mDbFailure.erase(entityId);
		co_return;
	}

	/// @brief save entity data list slow.
	void CheckSaveEntity(bool shutdown = false)
	{

		std::function<void(ClientEntityHelper::Ptr, bool)> dealFunc = nullptr;
		
		ClientProxyHelper::Ptr clientProxy = GetOwner()->GetComponent<ClientProxyHelper>(EMComponentType::ClientProxy);

		if (!clientProxy || clientProxy->RegistType() != uint8_t(EMServerType::GateServer))
		{
			dealFunc = [this](ClientEntityHelper::Ptr entity, bool offline)
				{
					std::string binData;
					size_t entityId = entity->ID();
					if (!entity->GetDbEntity())
					{
						LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "SaveEntity not pb Data:{}", entityId);
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
				LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "SaveEntity not pb Data:{}", ID);
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
	
};
