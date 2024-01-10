module;

#include <format>
#include <chrono>
export module AfxCommon;

export const char* GetNowTimeStr()
{
	static std::string dataFormate;
	static std::chrono::system_clock clock;
	dataFormate = std::format("{:%Y-%m-%d %H:%M:%S}", clock.now());
	return dataFormate.c_str();
}