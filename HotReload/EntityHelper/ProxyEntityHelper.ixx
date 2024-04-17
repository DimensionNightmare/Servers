module;

#include <string>
export module ProxyEntityHelper;

import DNServer;
export import ProxyEntity;
import EntityHelper;

using namespace std;

export class ProxyEntityHelper : public ProxyEntity
{
private:
	ProxyEntityHelper(){}
public:
	EntityHelper* GetChild(){ return nullptr;}

	string& Token(){ return sToken; }
	unsigned int & ExpireTime(){ return iExpireTime; }
};

