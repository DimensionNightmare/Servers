module;
export module Logger;

import StrUtils;
import L10nText;
import ThirdParty.PbGen;
import std.compat;
import ECSW;

class LogColor
{
public:
    inline static std::string RED	= "\033[31m";
    inline static std::string GREEN	= "\033[32m";
    inline static std::string YELLOW	= "\033[33m";
    inline static std::string BLUE	= "\033[34m";
    inline static std::string RESET	= "\033[0m";
};

export class LoggerPrint : public System
{
protected:
	friend class World;
	LoggerPrint(World::Ptr world):System(world)
	{
		eSystemType = EMSystemType::LoggerPrint;
	}
public:
	inline static ELogLevel SELogLevel = ELogLevel_Debug;

	template <typename... Args>
    static void Log(ELogLevel level, const std::format_string<Args...>& fmt, Args&&... args)
	{
		if (level < SELogLevel)
		{
			return;
		}

		std::string oResult = std::format("[{}] {} -> \n\t{}\n", 
			GetNowTimeStr(), 
			"", // olocation.function_name(),
			std::format(fmt, std::forward<Args>(args)...));
		
		switch (level)
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
	}

	static void Log(ELogLevel level, const std::string& fmt)
	{
		if (level < SELogLevel)
		{
			return;
		}

		std::string oResult = std::format("[{}] {} -> \n\t{}\n", 
			GetNowTimeStr(), 
			"", //olocation.function_name(), 
			fmt);

		switch (level)
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
	}

public:
	using Ptr = std::shared_ptr<LoggerPrint>;
	~LoggerPrint() = default;

	bool Awake() override
	{
		ELogLevel logLevel = ELogLevel_Debug;
		if(std::string* value = GetWorld()->LuanchParam("LoggerLevel"))
		{
			ELogLevel_Parse(*value, &logLevel);
		}

		std::filesystem::path logFile = *GetWorld()->LuanchParam("program");
		logFile = logFile.parent_path() / *GetWorld()->LuanchParam("svrName");
		SetLoggerLevel(logLevel, logFile);

		return true;
	}

	void flush()
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
	void operator()(EL10nCode code, Args&&... args)
	{
		const std::string& fmt = DNl10n::GetTipText(code, oLevel);

		if (oLevel < eLogLevel)
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
			"", // olocation.function_name(), 
			std::vformat(fmt, format_args));
		
		flush();
	}

	void operator()(EL10nCode code)
	{
		const std::string& fmt = DNl10n::GetTipText(code, oLevel);

		if (oLevel < eLogLevel)
		{
			return;
		}
		
		oResult = std::format("[{}] {} -> \n\t{}\n", 
			GetNowTimeStr(), 
			"", // olocation.function_name(),
			fmt);

		flush();
	}
	
	/// @brief set logger type and Log file Init 
	void SetLoggerLevel(std::optional<ELogLevel> level = std::nullopt, std::optional<std::filesystem::path> path = std::nullopt)
	{
		if(level)
		{
			eLogLevel = *level;
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
	std::string oResult;
	ELogLevel oLevel = ELogLevel_None;

protected:
	std::ofstream LogFile; 

	ELogLevel eLogLevel = ELogLevel_Normal;
};

