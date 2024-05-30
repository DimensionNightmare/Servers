module;
#include <string>
#include <cstdint>
#include "hv/Channel.h"
export module ProxyEntityHelper;

export import ProxyEntity;

using namespace std;
using namespace hv;

export class ProxyEntityHelper : public ProxyEntity
{
private:
	ProxyEntityHelper() = delete;
public:
	
};

