module;
#include "hv/Channel.h"
export module ServerEntity;

import Entity;
import DNServer;

using namespace hv;

export class ServerEntity : public Entity
{
    
public:
    ServerType emServerType;
};

module:private;