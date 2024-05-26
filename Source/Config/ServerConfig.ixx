module;
#include <map>
#include <string>
export module Config.Server;

using namespace std;

map<string, string>* LuanchConfig = nullptr;

export void SetLuanchConfig(map<string, string>* param)
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
