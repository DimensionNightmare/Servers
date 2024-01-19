module;

#include <assert.h>
export module DatabaseServerHelper;

import DatabaseServer;
import DNClientProxyHelper;
import ServerEntityManagerHelper;
import ServerEntity;

using namespace std;

export class DatabaseServerHelper : public DatabaseServer
{
private:
	DatabaseServerHelper(){};
public:

	DNClientProxyHelper* GetCSock(){ return nullptr;}
	ServerEntityManagerHelper<ServerEntity>* GetEntityManager(){ return nullptr;}
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