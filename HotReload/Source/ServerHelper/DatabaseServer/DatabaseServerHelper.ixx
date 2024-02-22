module;
#include "pqxx/connection"
#include "pqxx/transaction"
#include "GDef.pb.h"

#include <assert.h>
#include <format>
export module DatabaseServerHelper;

import DatabaseServer;
import DNClientProxyHelper;
import ServerEntityManagerHelper;
import ServerEntity;

import DNDbObj;
import AfxCommon;

using namespace std;
using namespace GDb;
using namespace google::protobuf;

#define DNPrint(code, level, fmt, ...) LoggerPrint(level, code, __FUNCTION__, fmt, ##__VA_ARGS__);

#define DBSelect(obj, name) .Select(#name, obj.name())
#define DBSelectCond(obj, name, cond, splicing) .SelectCond(obj, #name, cond, splicing, obj.name())
#define DBUpdate(obj, name) .Update(obj, #name, obj.name())
#define DBUpdateCond(obj, name, cond, splicing) .UpdateCond(obj, #name, cond, splicing, obj.name())
#define DBDeleteCond(obj, name, cond, splicing) .DeleteCond(obj, #name, cond, splicing, obj.name())

export class DatabaseServerHelper : public DatabaseServer
{
private:
	DatabaseServerHelper(){};
public:

	DNClientProxyHelper* GetCSock(){ return nullptr;}
	ServerEntityManagerHelper<ServerEntity>* GetEntityManager(){ return nullptr;}

	void InitDabase();
};

static DatabaseServerHelper* PDatabaseServerHelper = nullptr;

export void SetDatabaseServer(DatabaseServer* server)
{
	PDatabaseServerHelper = static_cast<DatabaseServerHelper*>(server);
	assert(PDatabaseServerHelper != nullptr);
}

export DatabaseServerHelper* GetDatabaseServer()
{
	return PDatabaseServerHelper;
}

void DatabaseServerHelper::InitDabase()
{
	try
	{
		//"postgresql://root@localhost"
		string* value = GetLuanchConfigParam("connection");
		pqxx::connection conn(*value);

		pqxx::work txn(conn);

		DNDbObj<Account> accountInfo(txn);
		if(!accountInfo.IsExist())
		{
			accountInfo.InitTable().Commit();
		}

		txn.commit();
	}
	catch(const exception& e)
	{
		DNPrint(-1, LoggerLevel::Error, "%s", e.what());
	}
}
