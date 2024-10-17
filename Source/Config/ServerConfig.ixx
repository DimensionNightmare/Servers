module;
#include "StdMacro.h"
export module Config.Server;

/// @brief pointer global (luanch param) and (ini config param)
unordered_map<string, string>* PInstance = nullptr;

/// @brief global addr set. main/dll set
export void SetLuanchConfig(unordered_map<string, string>* param)
{
	PInstance = param;
}

/// @brief global param get
export string* GetLuanchConfigParam(const char* key)
{
	if (!PInstance)
	{
		return nullptr;
	}

	auto res = PInstance->find(key);
	if (res != PInstance->end())
	{
		return &res->second;
	}

	return nullptr;
}
