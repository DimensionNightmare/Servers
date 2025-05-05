module;
export module Logger;

import StrUtils;
import L10nText;
export import ThirdParty.PbGen;
import ECSW;


class LoggerPrintPid;

class LogColor
{
public:
    inline static std::string RED		= "\033[31m";
    inline static std::string GREEN		= "\033[32m";
    inline static std::string YELLOW	= "\033[33m";
    inline static std::string BLUE		= "\033[34m";
    inline static std::string RESET		= "\033[0m" ;
};

export class LoggerPrint : public System
{
protected:
	friend class World;
	friend class LoggerPrintPid;
	LoggerPrint(World::Ptr world):System(world)
	{
		eSystemType = EMSystemType::LoggerPrint;
		pDNl10n = world->GetSystem<DNl10n>(EMSystemType::DNl10n);
	}
public:


public:
	using Ptr = std::shared_ptr<LoggerPrint>;
	~LoggerPrint() = default;

	bool Awake() override
	{
		if(std::string* value = GetWorld()->LuanchParam("LoggerLevel"))
		{
			ELogLevel logLevel = ELogLevel_Debug;
			ELogLevel_Parse(*value, &logLevel);
			SetLoggerLevel(logLevel, std::nullopt);
		}

		if(std::string* value = GetWorld()->LuanchParam("program"))
		{
			std::filesystem::path logFile = *value;
			logFile = logFile.parent_path() / *GetWorld()->LuanchParam("svrName");
			SetLoggerLevel(std::nullopt, logFile);
		}

		return true;
	}

	void flush(ELogLevel level, std::string& result)
	{
		if (result.empty())
		{
			return;
		}

		switch (level)
		{
			case ELogLevel_Normal:
				std::cout << LogColor::BLUE << result << LogColor::RESET;
				break;
			case ELogLevel_Warning:
				std::cout << LogColor::YELLOW << result << LogColor::RESET;
				break;
			case ELogLevel_Error:
				std::cout << LogColor::RED << result << LogColor::RESET;
				break;
			case ELogLevel_Debug:
				std::cout << result;
				break;
			default:
				return;
		}

		if (LogFile.is_open())
		{
			LogFile << result;
			LogFile.flush();
		}
	}

	template <typename... Args>
    void Record(ELogLevel level, const std::format_string<Args...>& fmt, Args&&... args)
	{
		if (level < eLogLevel)
		{
			return;
		}

		std::string result = std::format("[{}] {} -> \n\t{}\n", 
			GetNowTimeStr(), 
			"", // olocation.function_name(),
			std::format(fmt, std::forward<Args>(args)...));
		
		flush(level, result);
	}

	void Record(ELogLevel level, const std::string& fmt)
	{
		if (level < eLogLevel)
		{
			return;
		}

		std::string result = std::format("[{}] {} -> \n\t{}\n", 
			GetNowTimeStr(), 
			"", //olocation.function_name(), 
			fmt);

		flush(level, result);
	}

	template <typename... Args>
	void Record(EL10nCode code, Args&&... args)
	{
		ELogLevel level;
		const std::string& fmt = pDNl10n->GetTipText(code, level);

		if (level < eLogLevel)
		{
			return;
		}

		// std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
		auto&& args_tuple = std::forward_as_tuple(std::forward<Args>(args)...);

        auto format_args = std::apply([](auto&&... args) {
            return std::make_format_args(args...);
        }, args_tuple);
		
		std::string result = std::format("[{}] {} -> \n\t{}\n", 
			GetNowTimeStr(), 
			"", // olocation.function_name(), 
			std::vformat(fmt, format_args));
		
		flush(level, result);
	}

	void Record(EL10nCode code)
	{
		ELogLevel level;
		const std::string& fmt = pDNl10n->GetTipText(code, level);

		if (level < eLogLevel)
		{
			return;
		}
		
		std::string result = std::format("[{}] {} -> \n\t{}\n", 
			GetNowTimeStr(), 
			"", // olocation.function_name(),
			fmt);

		flush(level, result);
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
	std::ofstream LogFile; 

	ELogLevel eLogLevel = ELogLevel_Normal;

	DNl10n::Ptr pDNl10n;
};

class LoggerPrintPid
{
public:
	LoggerPrintPid()
	{
		pWorld = std::make_shared<World>();
		pWorld->AddSystem<DNl10n>();
		pLogger = pWorld->AddSystem<LoggerPrint>();
	}

	~LoggerPrintPid()
	{
		pWorld->Dispose();
		pWorld = nullptr;
	}

	void Init(std::optional<ELogLevel> level = std::nullopt, std::optional<std::filesystem::path> path = std::nullopt)
	{
		pLogger->SetLoggerLevel(level, path);
	}

	void Init(std::unordered_map<std::string, std::string> commonInfo)
	{
		pWorld->MoveLuanchConfigToSelf(std::move(commonInfo));
		DNl10n::Ptr pL10n = pWorld->GetSystem<DNl10n>(EMSystemType::DNl10n);
		if(const char* errInfo = pL10n->Init())
		{
			throw std::exception(errInfo);
		}
	}

	template <typename... Args>
    void Record(ELogLevel level, const std::format_string<Args...>& fmt, Args&&... args)
	{
		pLogger->Record(level, fmt, std::forward<Args>(args)...);
	}

	void Record(ELogLevel level, const std::string& fmt)
	{
		pLogger->Record(level, fmt);
	}

	template <typename... Args>
	void Record(EL10nCode code, Args&&... args)
	{
		pLogger->Record(code, std::forward<Args>(args)...);
	}

	void Record(EL10nCode code)
	{
		pLogger->Record(code);
	}

private:
	World::Ptr pWorld;
	LoggerPrint::Ptr pLogger;
};

export LoggerPrintPid SPidLogger;
