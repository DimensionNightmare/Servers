module;
#include "StdMacro.h"
export module Config.Server;

unordered_map<string, string>* LuanchConfig = nullptr;

export void SetLuanchConfig(unordered_map<string, string>* param)
{
	LuanchConfig = param;
}

export string* GetLuanchConfigParam(const char* key)
{
	if (!LuanchConfig)
	{
		return nullptr;
	}

	auto res = LuanchConfig->find(key);
	if (res != LuanchConfig->end())
	{
		return &res->second;
	}

	return nullptr;
}
