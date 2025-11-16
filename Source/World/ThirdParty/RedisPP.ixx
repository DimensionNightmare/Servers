module;
#include "sw/redis++/redis++.h"
export module ThirdParty.RedisPP;


export namespace sw::redis
{
	using sw::redis::Redis;
	using sw::redis::IoError;
	using sw::redis::Transaction;
};