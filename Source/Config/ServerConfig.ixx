module;
export module Config.Server;

import DllUtils;
import std.compat;

#define FUNCPLACE(func) #func, func

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
		static LaunchConfig* instance = PInstance ? PInstance.get() : GetDllInstance();
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

	static LaunchConfig* GetDllInstance()
	{
		return TickMainSpaceDll(PInstance.get(), FUNCPLACE(&LaunchConfig::GetInstance));
	}

public:

	inline static std::shared_ptr<LaunchConfig> PInstance = nullptr;

protected:
	/// @brief launch param
	std::unordered_map<std::string, std::string>* pLuanchParam = nullptr;

};
