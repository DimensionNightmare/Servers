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
import Server;

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
			ClientEntity::Ptr entity = GetBase()->AddEntity(entityId);
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

	Task<bool> LoadEntity(ClientEntityHelper::Ptr entity, GMsg::d2L_ReqLoadEntityData* inRequest, GMsg::L2d_ResLoadEntityData* inResponse)
	{
		ClientProxyHelper::CVPtr clientProxy = GetOwner()->GetComponent<ClientProxyHelper>(EMComponentType::ClientProxy);

		World::CVPtr world = GetWorld();

		if (!clientProxy || clientProxy->RegistType() != std::to_underlying(EMServerType::GateServer))
		{
			LoggerPrint::Log(world, ELogLevel_Debug, "client proxy not vaild or not gate server!");
			co_return false;
		}

		
		if (entity->HasFlag(EMClientEntityFlag::DBInited))
		{
			if(inResponse)
			{
				std::string* entitydata = inResponse->add_entitydata();
				GDb::Player* dbEntity = entity->GetDbEntity();
				if(!dbEntity->SerializeToString(entitydata))
				{
					LoggerPrint::Log(world, ELogLevel_Debug, "SerializeToString error on LoadEntity id = {}!", dbEntity->accountid());
					co_return false;
				}
			}
			co_return true;
		}
		else if(entity->HasFlag(EMClientEntityFlag::DBIniting))
		{
			LoggerPrint::Log(world, ELogLevel_Debug, "entity {} is DBIniting. return .", entity->ID());
			if (inResponse)
			{
				inResponse->set_errorcode(EL10nCode_DBIniting);
			}
			co_return false;
		}
		
		entity->SetFlag(EMClientEntityFlag::DBIniting);

		std::string binData;

		std::string tablename = GDb::Player::GetDescriptor()->full_name();
		size_t entityId = entity->ID();
		std::string keyName = std::format("{}_{}", tablename, entityId);

		MdbProxyHelper::CVPtr dbProxy = GetOwner()->GetComponent<MdbProxyHelper>(EMComponentType::MdbProxy);
		if(auto transaction = dbProxy->GetTransaction())
		{
			try
			{
				// nosql
                if (auto optValue = transaction->get(keyName).exec().get<sw::redis::OptionalString>(0))
				{
					binData = *optValue;
                }
			}
			catch(const std::exception& e)
			{
				LoggerPrint::Log(world, ELogLevel_Error, "entity Load Redis err {} ", e.what());
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
				co_return true;
			}
		}

		// sql
		auto request = std::make_shared<GMsg::L2D_ReqLoadData>();

		GDb::Player* dbEntity = entity->GetDbEntity();

		// only query db data
		if (inRequest)
		{
			request->set_tablename(inRequest->tablename());
			request->set_keynumber(inRequest->keynumber());
			request->set_entitydata(inRequest->entitydata());
			request->set_needcreate(inRequest->needcreate());
		}
		// this mean new Entity branch
		else
		{
			request->set_needcreate(true);

			request->set_limit(1);
			request->set_tablename(tablename);
			request->set_keynumber(GDb::Player::kAccountIdFieldNumber);
			
			dbEntity->SerializeToString(request->mutable_entitydata());
		}

		auto response = std::make_shared<GMsg::D2L_ResLoadData>();
			
		bool success = co_await clientProxy->AddMsg(EMMsgDeal::Redir, request.get(), response.get());

		if (!success)
		{
			response->set_errorcode(EL10nCode_CRdbReqTimeout);
		}

		entity->ClearFlag(EMClientEntityFlag::DBIniting);

		if (response->errorcode() != EL10nCode_None)
		{

			// binData = request->entitydata();
			// BytesToHexString(binData);
			// mDbFailure[entityId] = binData;
			LoggerPrint::Log(world, ELogLevel_Debug, "Load Db Entity Error id = {}, errorcode = {}! ", entityId, std::to_underlying(response->errorcode()));
			co_return false;
		}

		entity->SetFlag(EMClientEntityFlag::DBInited);

		int lenth = response->entitydata_size();
		if (lenth == 1)
		{
			const std::string& entityData = response->entitydata(0);
			entity->SetDbEntity(entityData);
			
			if(auto transaction = dbProxy->GetTransaction())
			{
				auto result = transaction->setex(keyName, 600, entityData)
					.exec().get<bool>(0);
				
			}

		}
		else if(lenth > 1)
		{
			LoggerPrint::Log(world, ELogLevel_Debug, "Load Db Entity mutiply data!");
		}


		if (inResponse)
		{
			inResponse->set_errorcode(response->errorcode());
			inResponse->mutable_entitydata()->Swap(response->mutable_entitydata());
		}

		co_return true;
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
			property_location->set_x(std::floor(property_location->x()));
			property_location->set_y(std::floor(property_location->y()));
			property_location->set_z(std::floor(property_location->z()));
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
			ClientProxyHelper::CVPtr clientProxy = GetOwner()->GetComponent<ClientProxyHelper>(EMComponentType::ClientProxy);

			bool success = co_await clientProxy->AddMsg(EMMsgDeal::Redir, &request, &response);

			if (!success)
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
		MdbProxyHelper::CVPtr dbProxy = GetOwner()->GetComponent<MdbProxyHelper>(EMComponentType::MdbProxy);
		if(auto transaction = dbProxy->GetTransaction())
		{
			std::string keyName = std::format("{}_{}", tablename, entityId);
			auto result = transaction->setex(keyName, 600, entitydata)
				.exec().get<bool>(0);
		}

		mDbFailure.erase(entityId);
		co_return;
	}

	/// @brief save entity data list slow.
	void CheckSaveEntity(bool shutdown = false)
	{

		std::function<void(ClientEntityHelper::CVPtr, bool)> dealFunc;
		
		ClientProxyHelper::CVPtr clientProxy = GetOwner()->GetComponent<ClientProxyHelper>(EMComponentType::ClientProxy);

		if (!clientProxy || clientProxy->RegistType() != std::to_underlying(EMServerType::GateServer))
		{
			dealFunc = [this](ClientEntityHelper::CVPtr entity, bool offline)
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

		auto entitys = mEntityMap
			| std::views::values
			| std::views::filter([](const auto& entity)
				{
					return entity->GetDbEntity() != nullptr;
				})
			| std::views::transform([](const auto& entity)
				{
					return entity->GetSelf<ClientEntityHelper>();
				});

		for (const auto& entity : entitys)
		{
			if(shutdown)
			{
				dealFunc(entity, shutdown);
				continue;
			}

			if (entity->HasFlag(EMClientEntityFlag::DBModify))
			{
				entity->ClearFlag(EMClientEntityFlag::DBModify);

				dealFunc(entity, shutdown);
			}
		}
	}
	

	size_t GetSaveTimerId()
	{
		return iSaveTimerId;
	}

	void SetSaveTimerId(size_t timerId)
	{
		iSaveTimerId = timerId;
	}
};
