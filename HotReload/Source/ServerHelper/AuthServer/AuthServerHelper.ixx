module;

#include <assert.h>
export module AuthServerHelper;

import AuthServer;
import DNClientProxyHelper;
import DNWebProxyHelper;

using namespace std;

class AuthServerHelper : public AuthServer
{
private:
	AuthServerHelper(){}
public:
	DNClientProxyHelper* GetCSock(){ return nullptr;}
	DNWebProxyHelper* GetSSock(){ return nullptr;}
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