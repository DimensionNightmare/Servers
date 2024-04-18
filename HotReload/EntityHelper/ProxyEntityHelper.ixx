module;
#include <string>
export module ProxyEntityHelper;

export import ProxyEntity;
import DNEntityHelper;

using namespace std;

export class ProxyEntityHelper : public ProxyEntity
{
private:
	ProxyEntityHelper(){}
public:
	DNEntityHelper* GetChild(){ return nullptr;}

	string& Token(){ return sToken; }
	unsigned int & ExpireTime(){ return iExpireTime; }
};

