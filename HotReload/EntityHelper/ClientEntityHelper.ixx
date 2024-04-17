module;

#include <string>
export module ClientEntityHelper;

import DNServer;
export import ClientEntity;
import EntityHelper;

using namespace std;

export class ClientEntityHelper : public ClientEntity
{
private:
	ClientEntityHelper(){}
public:
	EntityHelper* GetChild(){ return nullptr;}

	
};

