module;
#include <unordered_map>
#include <list>
#include <shared_mutex>
#include <cstdint>
#include <coroutine>
#include "sw/redis++/redis++.h"

#include "StdMacro.h"
#include "GDef/GDef.pb.h"
#include "Server/S_Logic.pb.h"
export module ClientEntityManager;

export import ClientEntity;
export import DNServer;
import EntityManager;
import Logger;
import DbUtils;
import DNClientProxy;
import DNTask;
import MessagePack;

using namespace std;
using namespace sw::redis;
using namespace GDb;
using namespace google::protobuf;
using namespace GMsg;

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

	DNTaskVoid SaveEntity(ClientEntity& entity);

	void CheckSaveEntity();

protected: // dll proxy
	shared_ptr<Redis> pNoSqlProxy;
	DNClientProxy* pSqlClient;

	unordered_map<uint32_t, string> mDbFailure;
};

ClientEntityManager::~ClientEntityManager()
{
	CheckSaveEntity();
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

DNTaskVoid ClientEntityManager::SaveEntity(ClientEntity& entity)
{
	uint32_t entityId = entity.ID();

	if(!entity.pDbEntity)
	{
		DNPrint(0, LoggerLevel::Debug, "SaveEntity not pb Data:%u", entityId);
		co_return;
	}

	
	auto& dbEntity =  *entity.pDbEntity;

	string entity_data;
	dbEntity.SerializeToString(&entity_data);

	// sql
	L2D_ReqSaveData request;
	string table_name = dbEntity.GetDescriptor()->full_name();
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

	if(int code = response.state_code())
	{
		mDbFailure[entityId] = binData;
		DNPrint(0, LoggerLevel::Debug, "Save Db Entity Error id = %u, state_code = %d! ", entityId, code);
		co_return;
	}

	// nosql
	string keyName = format("{}_{}", table_name, entityId);
	pNoSqlProxy->set(keyName, entity_data);

	mDbFailure.erase(entityId);
	co_return;
}

void ClientEntityManager::CheckSaveEntity()
{

	function<void(ClientEntity&)> dealFunc = nullptr;

	if(!pSqlClient || pSqlClient->RegistType() != uint8_t(ServerType::GateServer) || !pNoSqlProxy)
	{
		dealFunc = [this](ClientEntity& entity)
			{
				string binData;
				uint32_t entityId = entity.ID();
				if(!entity.pDbEntity)
				{
					DNPrint(0, LoggerLevel::Debug, "SaveEntity not pb Data:%u", entityId);
					return;
				}

				entity.pDbEntity->SerializeToString(&binData);
				mDbFailure[entityId] = binData;
			};
	}
	else
	{
		dealFunc = std::bind(&ClientEntityManager::SaveEntity, this, std::placeholders::_1);
	}

	for (auto& [ID, entity] : mEntityMap)
	{
		if (entity.HasFlag(ClientEntityFlag::DBModify))
		{
			entity.ClearFlag(ClientEntityFlag::DBModify);

			dealFunc(entity);
		}
	}
}

void ClientEntityManager::TickMainFrame()
{
	CheckSaveEntity();
}
