module;

#include <string>
#include <array>
#include <format>
#include <chrono>
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
