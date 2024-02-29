module;

#include <string>
export module ProxyEntityHelper;

import DNServer;
export import ProxyEntity;
import EntityHelper;

using namespace std;

export enum class ProxyEntityFlag : int
{
};

export class ProxyEntityHelper : public ProxyEntity
{
private:
	ProxyEntityHelper(){}
public:
	EntityHelper* GetChild(){ return nullptr;}

	bool HasFlag(ProxyEntityFlag flag){ return oFlags.test(int(flag));}
	void SetFlag(ProxyEntityFlag flag){ oFlags.set(int(flag));}
	void ClearFlag(ProxyEntityFlag flag){ oFlags.reset(int(flag));}
	
	string& Token(){ return sToken; }
	unsigned int & ExpireTime(){ return iExpireTime; }
};

module:private;