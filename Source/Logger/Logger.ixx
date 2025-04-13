module;
export module Logger;

import StrUtils;
import L10nText;
import ThirdParty.PbGen;
import std.compat;

class LogColor {
public:
    inline static std::string RED	= "\033[31m";
    inline static std::string GREEN	= "\033[32m";
    inline static std::string YELLOW	= "\033[33m";
    inline static std::string BLUE	= "\033[34m";
    inline static std::string RESET	= "\033[0m";
};

export struct LoggerPrint
{
	LoggerPrint(const std::source_location& location = std::source_location::current())
		:olocation(location)
    {	
	}

	~LoggerPrint()
	{
		if (oResult.empty())
		{
			return;
		}

		switch (oLevel)
		{
			case ELogLevel_Normal:
				std::cout << LogColor::BLUE << oResult << LogColor::RESET;
				break;
			case ELogLevel_Warning:
				std::cout << LogColor::YELLOW << oResult << LogColor::RESET;
				break;
			case ELogLevel_Error:
				std::cout << LogColor::RED << oResult << LogColor::RESET;
				break;
			case ELogLevel_Debug:
				std::cout << oResult;
				break;
			default:
				return;
		}

		if (LogFile.is_open())
		{
			LogFile << oResult;
			LogFile.flush();
		}
	}

	template <typename... Args>
    void operator()(ELogLevel level, const std::format_string<Args...>& fmt, Args&&... args)
	{
		oLevel = level;

		if (level < SLogLevel)
		{
			return;
		}

		std::string* locCache = nullptr;
		// if(LocCache.contains())
		// {

		// }

		oResult = std::format("[{}] {} -> \n\t{}\n", 
			GetNowTimeStr(), 
			olocation.function_name(),
			std::format(fmt, std::forward<Args>(args)...));
	}

	template <typename... Args>
	void operator()(EL10nCode code, Args&&... args)
	{
		const std::string& fmt = DNl10n::GetTipText(code, oLevel);

		if (oLevel < SLogLevel)
		{
			return;
		}

		// std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
		auto&& args_tuple = std::forward_as_tuple(std::forward<Args>(args)...);

        auto format_args = std::apply([](auto&&... args) {
            return std::make_format_args(args...);
        }, args_tuple);
		
		oResult = std::format("[{}] {} -> \n\t{}\n", 
			GetNowTimeStr(), 
			olocation.function_name(), 
			std::vformat(fmt, format_args));
	}

	void operator()(EL10nCode code)
	{
		const std::string& fmt = DNl10n::GetTipText(code, oLevel);

		if (oLevel < SLogLevel)
		{
			return;
		}
		
		oResult = std::format("[{}] {} -> \n\t{}\n", 
			GetNowTimeStr(), 
			olocation.function_name(),
			fmt);
	}

	void operator()(ELogLevel level, const std::string& fmt)
	{
		oLevel = level;

		if (level < SLogLevel)
		{
			return;
		}

		oResult = std::format("[{}] {} -> \n\t{}\n", 
			GetNowTimeStr(), 
			olocation.function_name(), 
			fmt);
	}

	/// @brief set logger type and Log file Init 
	static void SetLoggerLevel(std::optional<ELogLevel> level = std::nullopt, std::optional<std::filesystem::path> path = std::nullopt)
	{
		if(level)
		{
			SLogLevel = *level;
		}
		
		if(!LogFile.is_open() && path)
		{
			if(!std::filesystem::exists(*path))
			{
				std::filesystem::create_directories(*path);
			}
			LogFile = std::ofstream( std::format("{}/Output.log", path.value().string()), std::ios::app);
		}
	}

protected:
	const std::source_location& olocation;
	std::string oResult;
	ELogLevel oLevel = ELogLevel_None;

protected:
	inline static std::ofstream LogFile; 
	inline static ELogLevel SLogLevel = ELogLevel_Normal;
	inline static std::unordered_map<std::string, std::string> LocCache;
};

