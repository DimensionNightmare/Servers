module;

#include <format>
#include <chrono>
export module AfxCommon;

export std::string GetNowTimeStr()
{
	std::chrono::system_clock clock;
	return std::format("{:%Y-%m-%d %H:%M:%S}", clock.now());
}