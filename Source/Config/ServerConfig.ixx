module;
#include "StdMacro.h"
export module Config.Server;

import DllUtils;

export class LaunchConfig
{
public:

	LaunchConfig()
	{
	}

	~LaunchConfig()
	{
		pLuanchParam = nullptr;
	}

	void SetLuanchConfig(std::unordered_map<std::string, std::string>& param)
	{
		pLuanchParam = &param;
	}

	/// @brief global param get
	static std::string* GetParam(const char* key)
	{
		static std::shared_ptr<LaunchConfig> instance = PInstance ? PInstance : GetDllInstance();
		if (!instance)
		{
			return nullptr;
		}

		auto res = instance->pLuanchParam->find(key);
		if (res != instance->pLuanchParam->end())
		{
			return &res->second;
		}

		return nullptr;
	}

	LaunchConfig* GetInstance()
	{
		return PInstance.get();
	}

protected:

	static std::shared_ptr<LaunchConfig> GetDllInstance()
	{
		if(LaunchConfig* handle = TICK_MAINSPACE_SIGN_FUNCTION(LaunchConfig, GetInstance, PInstance.get()))
		{
			return std::shared_ptr<LaunchConfig>(handle, [](LaunchConfig* obj){});
		}
		return nullptr;
	}

public:

	inline static std::shared_ptr<LaunchConfig> PInstance = nullptr;

protected:
	/// @brief launch param
	std::unordered_map<std::string, std::string>* pLuanchParam = nullptr;

};
