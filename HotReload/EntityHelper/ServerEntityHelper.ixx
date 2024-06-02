module;
#include <string>
#include <cstdint>
#include <list>
#include "hv/Channel.h"
export module ServerEntityHelper;

export import ServerEntity;

using namespace std;
using namespace hv;

export class ServerEntityHelper : public ServerEntity
{
private:
	ServerEntityHelper() = delete;
public:

};

