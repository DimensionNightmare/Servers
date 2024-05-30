module;
#include <cstdint>
#include <string>
#include <array>
#include <format>
#include <chrono>
#include <regex>
#ifdef _WIN32
#include <timezoneapi.h>
#endif
export module StrUtils;

using namespace std;

export template <auto value>
constexpr auto EnumName()
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
	return start == string_view::npos ? name : string_view{ name.data() + start + 2, name.size() - start - 2 };
}

template <typename T, size_t N = 0>
constexpr auto EnumMax()
{
	constexpr auto value = static_cast<T>(N);
	if constexpr (EnumName<value>().find(")") == string_view::npos)
	{
		return EnumMax<T, N + 1>();
	}
	else
	{
		return N;
	}
}

/// @brief enum class must continue
/// @tparam T
/// @param value
/// @return
export template <typename T>
	requires is_enum_v<T>
constexpr auto EnumName(T value)
{
	constexpr auto num = EnumMax<T>();
	constexpr auto names = []<size_t... Is>(index_sequence<Is...>)
	{
		return array<string_view, num>
		{
			EnumName<static_cast<T>(Is)>()...
		};
	}(make_index_sequence<num>{});
	return names[static_cast<size_t>(value)];
}

export template <typename T>
	requires is_enum_v<T>
constexpr auto EnumName(string_view value)
{
	constexpr auto num = EnumMax<T>();
	constexpr auto names = []<size_t... Is>(index_sequence<Is...>)
	{
		return array<string_view, num>
		{
			EnumName<static_cast<T>(Is)>()...
		};
	}(make_index_sequence<num>{});

	auto it = find(names.begin(), names.end(), value);
    if (it != names.end()) {
        return static_cast<T>(distance(names.begin(), it));
    }
    throw invalid_argument("Unknown enum name");
}

export string GetNowTimeStr()
{
	using namespace std::chrono;
	return format("{:%Y-%m-%d %H:%M:%S}", zoned_time(current_zone(), system_clock::now()));
}

export double StringToTimestamp(const string& datetimeStr)
{

	regex pattern(R"((\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\.?(\d{6})?(\+|-)(\d{2}))");
	smatch match;

	if (!regex_match(datetimeStr, match, pattern))
	{
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

#pragma region MD5

#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))

const uint32_t MD5_INIT_CONSTANTS[] =
{
	0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476
};

const int MD5_ROTATE_COUNT[] =
{
	7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
	5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
	4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
	6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

const uint32_t MD5_CONSTANTS[] =
{
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
	0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
	0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
	0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
	0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
	0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
	0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
	0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
	0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

string PaddingMessage(const string& message)
{
	uint64_t messageLength = message.length() * 8;
	uint64_t paddingLength = (messageLength % 512 < 448) ? (448 - messageLength % 512) : (960 - messageLength % 512);
	paddingLength /= 8;

	string paddedMessage = message;
	paddedMessage += '\x80';
	paddedMessage.append(paddingLength - 1, '\0');

	for (int i = 0; i < 8; ++i)
	{
		paddedMessage += (char)((messageLength >> (i * 8)) & 0xFF);
	}

	return paddedMessage;
}

export string Md5Hash(const string& message)
{
	uint32_t a = MD5_INIT_CONSTANTS[0];
	uint32_t b = MD5_INIT_CONSTANTS[1];
	uint32_t c = MD5_INIT_CONSTANTS[2];
	uint32_t d = MD5_INIT_CONSTANTS[3];

	string paddedMessage = PaddingMessage(message);
	const uint32_t* chunks = reinterpret_cast<const uint32_t*>(paddedMessage.c_str());

	for (size_t i = 0; i < paddedMessage.length() / 64; ++i)
	{
		uint32_t aTemp = a;
		uint32_t bTemp = b;
		uint32_t cTemp = c;
		uint32_t dTemp = d;

		for (int j = 0; j < 64; ++j)
		{
			uint32_t f, g;
			if (j < 16)
			{
				f = (bTemp & cTemp) | ((~bTemp) & dTemp);
				g = j;
			}
			else if (j < 32)
			{
				f = (dTemp & bTemp) | ((~dTemp) & cTemp);
				g = (5 * j + 1) % 16;
			}
			else if (j < 48)
			{
				f = bTemp ^ cTemp ^ dTemp;
				g = (3 * j + 5) % 16;
			}
			else
			{
				f = cTemp ^ (bTemp | (~dTemp));
				g = (7 * j) % 16;
			}

			uint32_t temp = dTemp;
			dTemp = cTemp;
			cTemp = bTemp;
			bTemp = bTemp + LEFTROTATE((aTemp + f + MD5_CONSTANTS[j] + chunks[i * 16 + g]), MD5_ROTATE_COUNT[j]);
			aTemp = temp;
		}

		a += aTemp;
		b += bTemp;
		c += cTemp;
		d += dTemp;
	}

	string result;
	result.reserve(16);
	for (uint32_t val : {a, b, c, d})
	{
		for (int i = 0; i < 4; ++i)
		{
			result += (char)((val >> (i * 8)) & 0xFF);
		}
	}

	ostringstream oss;
	oss << hex << setfill('0');
	for (uint8_t ch : result)
	{
		oss << setw(2) << static_cast<uint32_t>(ch);
	}
	return oss.str();
}

#pragma endregion