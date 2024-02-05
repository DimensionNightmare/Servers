module;
#include "pqxx/connection"
#include "pqxx/transaction"
#include "GDef.pb.h"

#include <assert.h>
export module DatabaseServerHelper;

import DatabaseServer;
import DNClientProxyHelper;
import ServerEntityManagerHelper;
import ServerEntity;

import DNDbObj;

using namespace std;
using namespace GDef;
using namespace google::protobuf;

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
		pqxx::connection c("host=localhost port=5432 dbname=postgres user=postgres password=1270 sslmode=prefer connect_timeout=10");
		c.set_client_encoding("GBK");
		pqxx::work txn(c);
		int result = txn.query_value<int>("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = 'public'");
		// not table empty
		// if(! result[0][0].as<int>())
		{
			DNDbObj<Account> accountInfo(txn);
			// accountInfo.InitTable().Commit();

			list<Account> datas;
			Account temp;
			temp.set_accountid(1);
			temp.set_createtime(123123);
			temp.set_updatetime(324);
			temp.set_authstring("2342");
			temp.set_authname("zxcxfd");
			temp.set_email("iopi");

			datas.emplace_back(temp);
			temp.set_accountid(2);
			datas.emplace_back(temp);
			temp.set_accountid(3);
			datas.emplace_back(temp);

			// accountInfo.AddRecord(datas).Commit();

			accountInfo
				.Query(-1)
				// .Query(0, 0, " %s > 2")
				.Query(-2,2, " %s = 324")
				.Commit();

			for(Account& msg : accountInfo.Result())
			{
				printf("%s \n\n", msg.DebugString().c_str());
			}
			
		}

		txn.commit();
	}
	catch(const exception& e)
	{
		printf("%s \n", e.what());
	}
}
