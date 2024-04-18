module;
#include <string>
export module ClientEntityHelper;

export import ClientEntity;
import DNEntityHelper;

using namespace std;

export class ClientEntityHelper : public ClientEntity
{
private:
	ClientEntityHelper(){}
public:
	DNEntityHelper* GetChild(){ return nullptr;}

	
};

