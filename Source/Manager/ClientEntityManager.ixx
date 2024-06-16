module;
#include <unordered_map>
#include <list>
#include <shared_mutex>
#include <cstdint>
#include <coroutine>
#include <format>
#include <functional>

#include "StdMacro.h"
export module ClientEntityManager;

export import ClientEntity;
export import DNServer;
import EntityManager;
import Logger;
import DNClientProxy;
import DNTask;
import MessagePack;
import StrUtils;
import ThirdParty.PbGen;
import ThirdParty.RedisPP;

export class ClientEntityManager : public EntityManager<ClientEntity>
{
public:
	ClientEntityManager() = default;

	virtual ~ClientEntityManager();

	virtual bool Init() override;

	void InitSqlConn(const shared_ptr<Redis>& redisConn);

	void InitSqlConn(DNClientProxy* sockClient);

	virtual void TickMainFrame() override;

public: // dll override

	DNTaskVoid SaveEntity(ClientEntity& entity, bool offline = false);

	void CheckSaveEntity(bool shutdown = false);

protected: // dll proxy
	shared_ptr<Redis> pNoSqlProxy;
	DNClientProxy* pSqlClient;

	unordered_map<uint32_t, string> mDbFailure;
};

ClientEntityManager::~ClientEntityManager()
{
	CheckSaveEntity(true);
}

bool ClientEntityManager::Init()
{
	return EntityManager::Init();
}

void ClientEntityManager::InitSqlConn(const shared_ptr<Redis>& redisConn)
{
	pNoSqlProxy = redisConn;
}

void ClientEntityManager::InitSqlConn(DNClientProxy* sockClient)
{
	pSqlClient = sockClient;
}

DNTaskVoid ClientEntityManager::SaveEntity(ClientEntity& entity, bool offline)
{
	uint32_t entityId = entity.ID();

	Player* dbEntity = entity.GetDbEntity();

	DNPrint(0, LoggerLevel::Debug, "end1:%s!", dbEntity->DebugString().c_str());

	// change maprecord
	if(offline)
	{
		auto mapInfo = dbEntity->mutable_map_info();
		auto cur_point = mapInfo->mutable_cur_point();
		Vector3* property_location = dbEntity->mutable_property_entity()->mutable_location();
		*cur_point->mutable_point() = *property_location;
		property_location->Clear();

		auto last_point = mapInfo->mutable_last_point();
		*last_point = *cur_point;
		last_point->Clear();
	}

	string entity_data;
	dbEntity->SerializeToString(&entity_data);

	// sql
	L2D_ReqSaveData request;
	string table_name = dbEntity->GetDescriptor()->full_name();
	request.set_table_name(table_name);
	request.set_key_name(ClientEntity::SKeyName);
	request.set_entity_data(entity_data);

	D2L_ResSaveData response;

	uint32_t msgId = pSqlClient->GetMsgId();

	string binData;
	request.SerializeToString(&binData);
	MessagePack(msgId, MsgDeal::Redir, request.GetDescriptor()->full_name().c_str(), binData);

	{
		auto taskGen = [](Message* msg) -> DNTask<Message*>
			{
				co_return msg;
			};
		auto dataChannel = taskGen(&response);
		pSqlClient->AddMsg(msgId, &dataChannel, 9000);
		pSqlClient->send(binData);
		co_await dataChannel;
		if (dataChannel.HasFlag(DNTaskFlag::Timeout))
		{
			response.set_state_code(10);
			DNPrint(0, LoggerLevel::Debug, "requst timeout! ");
		}
	}

	if (int code = response.state_code())
	{
		BytesToHexString(entity_data);
		mDbFailure[entityId] = entity_data;
		DNPrint(0, LoggerLevel::Debug, "Save Db Entity Error id = %u, state_code = %d! ", entityId, code);
		co_return;
	}

	// nosql
	string keyName = format("{}_{}", table_name, entityId);
	pNoSqlProxy->set(keyName, entity_data);

	mDbFailure.erase(entityId);
	co_return;
}

void ClientEntityManager::CheckSaveEntity(bool shutdown)
{

	function<void(ClientEntity&, bool)> dealFunc = nullptr;

	if (!pSqlClient || pSqlClient->RegistType() != uint8_t(ServerType::GateServer) || !pNoSqlProxy)
	{
		dealFunc = [this](ClientEntity& entity, bool offline)
			{
				string binData;
				uint32_t entityId = entity.ID();
				if (!entity.pDbEntity)
				{
					DNPrint(0, LoggerLevel::Debug, "SaveEntity not pb Data:%u", entityId);
					return;
				}

				entity.pDbEntity->SerializeToString(&binData);

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
		if (!entity.pDbEntity)
		{
			DNPrint(0, LoggerLevel::Debug, "SaveEntity not pb Data:%u", ID);
			continue;
		}

		if(shutdown)
		{
			dealFunc(entity, shutdown);
			continue;
		}

		if (entity.HasFlag(ClientEntityFlag::DBModify))
		{
			entity.ClearFlag(ClientEntityFlag::DBModify);

			dealFunc(entity, shutdown);
		}
	}
}

void ClientEntityManager::TickMainFrame()
{
	CheckSaveEntity();
}
