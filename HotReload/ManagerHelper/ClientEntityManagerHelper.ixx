module;
#include <shared_mutex>
#include <cstdint>
#include <coroutine>
#include <format>

#include "StdMacro.h"
#include "Server/S_Logic.pb.h"
export module ClientEntityManagerHelper;

export import ClientEntityHelper;
export import ClientEntityManager;
import Logger;
import DNTask;
import MessagePack;

using namespace std;
using namespace GMsg;
using namespace google::protobuf;

export class ClientEntityManagerHelper : public ClientEntityManager
{
private:
	ClientEntityManagerHelper() = delete;
public:
	ClientEntity* AddEntity(uint32_t entityId);

	bool RemoveEntity(uint32_t entityId);

	ClientEntity* GetEntity(uint32_t entityId);

	DNTaskVoid LoadEntityData(ClientEntity* entity, bool needCreate = false);

	void ClearNosqlProxy() { pNoSqlProxy = nullptr; }
};

ClientEntity* ClientEntityManagerHelper::AddEntity(uint32_t entityId)
{
	if (!mEntityMap.contains(entityId))
	{
		unique_lock<shared_mutex> ulock(oMapMutex);
		mEntityMap.emplace(std::piecewise_construct,
			std::forward_as_tuple(entityId),
			std::forward_as_tuple(entityId));
		ClientEntity* entity = &mEntityMap[entityId];
		LoadEntityData(entity, true);
		return entity;
	}

	return nullptr;
}

bool ClientEntityManagerHelper::RemoveEntity(uint32_t entityId)
{

	if (mEntityMap.contains(entityId))
	{
		unique_lock<shared_mutex> ulock(oMapMutex);

		DNPrint(0, LoggerLevel::Debug, "destory client entity");
		mEntityMap.erase(entityId);

		return true;
	}

	return false;
}


ClientEntity* ClientEntityManagerHelper::GetEntity(uint32_t entityId)
{
	shared_lock<shared_mutex> lock(oMapMutex);
	if (mEntityMap.contains(entityId))
	{
		return &mEntityMap[entityId];
	}
	// allow return empty
	return nullptr;
}

DNTaskVoid ClientEntityManagerHelper::LoadEntityData(ClientEntity* entity, bool needCreate)
{
	if(!pSqlClient || pSqlClient->RegistType() != uint8_t(ServerType::GateServer) || !pNoSqlProxy)
	{
		co_return;
	}

	auto& dbEntity = *entity->pDbEntity;
	string table_name = dbEntity.GetDescriptor()->full_name();
	uint32_t entityId = entity->ID();
	string keyName = format("{}_{}", table_name, entityId);
	
	// nosql

	string binData;
	if(auto res = pNoSqlProxy->get(keyName))
	{
		binData = res.value();
	}
	
	if(!binData.empty())
	{
		dbEntity.ParseFromString(binData);
		entity->SetFlag(ClientEntityFlag::DBInited);
		co_return;
	}

	entity->SetFlag(ClientEntityFlag::DBIniting);
	// sql
	L2D_ReqLoadData request;
	request.set_table_name(table_name);
	request.set_key_name(ClientEntity::SKeyName);
	request.set_limit(1);
	request.set_need_create(needCreate);
	
	string* entity_data = request.mutable_entity_data();
	dbEntity.SerializeToString(entity_data);

	request.SerializeToString(&binData);

	D2L_ResLoadData response;

	uint32_t msgId = pSqlClient->GetMsgId();
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

	if(int code = response.state_code())
	{
		entity->ClearFlag(ClientEntityFlag::DBIniting);
		mDbFailure[entityId] = *entity_data;
		DNPrint(0, LoggerLevel::Debug, "Load Db Entity Error id = %u, state_code = %d! ", entityId, code);
		co_return;
	}

	if(int lenth = response.entity_data_size(); lenth == 1)
	{
		const string entityData = response.entity_data(0);
		dbEntity.ParseFromString(entityData);
		entity->SetFlag(ClientEntityFlag::DBInited);

		pNoSqlProxy->set(keyName, entityData);
	}
	else
	{
		DNPrint(0, LoggerLevel::Debug, "Load Db Entity empty data!");
	}

	entity->ClearFlag(ClientEntityFlag::DBIniting);

	co_return;
}
