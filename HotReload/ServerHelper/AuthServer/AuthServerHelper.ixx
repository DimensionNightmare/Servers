module;
#include <assert.h>
#include "pqxx/connection"
#include "pqxx/transaction"

#include "StdAfx.h"
#include "GDef/GDef.pb.h"
export module AuthServerHelper;

import AuthServer;
import DNClientProxyHelper;
import DNWebProxyHelper;
import DNDbObj;

using namespace std;
using namespace GDb;

export class AuthServerHelper : public AuthServer
{
private:
	AuthServerHelper(){}
public:
	DNClientProxyHelper* GetCSock(){ return nullptr;}
	DNWebProxyHelper* GetSSock(){ return nullptr;}
	bool InitDatabase();
};

static AuthServerHelper* PAuthServerHelper = nullptr;

export void SetAuthServer(AuthServer* server)
{
	PAuthServerHelper = static_cast<AuthServerHelper*>(server);
	assert(PAuthServerHelper != nullptr);
}

export AuthServerHelper* GetAuthServer()
{
	return PAuthServerHelper;
}

bool AuthServerHelper::InitDatabase()
{
	try
	{
		//"postgresql://root@localhost"
		string* value = GetLuanchConfigParam("connection");
		pqxx::connection* conn = new pqxx::connection(*value);
		conn->set_client_encoding("UTF8");
		pqxx::work txn(*conn);

		DNDbObj<Account> accountInfo(&txn);
		if(!accountInfo.IsExist())
		{
			accountInfo.InitTable().Commit();
		}

		txn.commit();

		SetDbConnection(conn);
	}
	catch(const exception& e)
	{
		DNPrint(0, LoggerLevel::Debug, "%s", e.what());
		return false;
	}

	return true;
}
