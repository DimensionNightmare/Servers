module;
#include "sw/redis++/redis++.h"
export module ThirdParty.RedisPP;

using namespace sw::redis;

export 
{
	using ::Redis;
};