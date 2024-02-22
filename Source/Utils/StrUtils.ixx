module;

#include <string>
#include <array>
#include <format>
#include <chrono>
#include <regex>
export module Utils.StrUtils;

using namespace std;

export template<auto value>
constexpr auto enum_name()
{
    string_view name;
#if __GNUC__ || __clang__
    name = __PRETTY_FUNCTION__;
    size_t start = name.find('=') + 2;
    size_t end = name.size() - 1;
    name = string_view{ name.data() + start, end - start };
    start = name.rfind("::");
#elif _MSC_VER
    name = __FUNCSIG__;
    size_t start = name.find('<') + 1;
    size_t end = name.rfind(">(");
    name = string_view{ name.data() + start, end - start };
    start = name.rfind("::");
#endif
    return start == string_view::npos ? name : string_view{
            name.data() + start + 2, name.size() - start - 2
    };
}

template<typename T, size_t N = 0> 
constexpr auto enum_max()
{
    constexpr auto value = static_cast<T>(N);
    if constexpr (enum_name<value>().find(")") == string_view::npos)
        return enum_max<T, N + 1>();
    else
        return N;
}

/// @brief enum class must continue
/// @tparam T 
/// @param value 
/// @return 
export template<typename T> requires is_enum_v<T>
constexpr auto enum_name(T value)
{
    constexpr auto num = enum_max<T>();
    constexpr auto names = []<size_t... Is>(index_sequence<Is...>)
    {
        return array<string_view, num>
        { 
            enum_name<static_cast<T>(Is)>()... 
        };
    }(make_index_sequence<num>{});
    return names[static_cast<size_t>(value)];
}

export string GetNowTimeStr()
{
	chrono::system_clock clock;
	return format("{:%Y-%m-%d %H:%M:%S}", clock.now());
}

export double StringToTimestamp(const string& datetimeStr)
{

    regex pattern(R"((\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\.?(\d{6})?(\+|-)(\d{2}))");
    smatch match;

    if (!regex_match(datetimeStr, match, pattern)) {
        throw runtime_error("Invalid datetime format");
    }

    string datetime = match[1];

    string microseconds_str = match[2];

    int timezone_offset = stoi(match[4]);

    tm tm = {};
    stringstream ss(datetime);
    ss >> get_time(&tm, "%Y-%m-%d %H:%M:%S");

    auto tp = chrono::system_clock::from_time_t(mktime(&tm));

    if (!microseconds_str.empty()) 
	{
        int microseconds = stoi(microseconds_str);
        tp += chrono::microseconds(microseconds);
    }

    // time zone
    // tp -= chrono::hours(timezone_offset);

	double timestamp = chrono::duration<double>(tp.time_since_epoch()).count();
    return timestamp;
}