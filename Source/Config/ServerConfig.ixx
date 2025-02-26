module;
#include "StdMacro.h"
export module Config.Server;

/// @brief pointer global (luanch param) and (ini config param)
std::unordered_map<std::string, std::string>* PInstance = nullptr;

/// @brief global addr set. main/dll set
export void SetLuanchConfig(std::unordered_map<std::string, std::string>* param)
{
	PInstance = param;
}

/// @brief global param get
export std::string* GetLuanchConfigParam(const char* key)
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
