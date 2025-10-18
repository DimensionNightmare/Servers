export module Logger;

import StrUtils;
import L10nText;
import ECSW;
import ThirdParty.Protobuf;
import std.compat;
import ThirdParty.Platform;

namespace LogColor
{
    const std::string RED		= "\033[31m";
    const std::string GREEN		= "\033[32m";
    const std::string YELLOW	= "\033[33m";
    const std::string BLUE		= "\033[34m";
    const std::string RESET		= "\033[0m" ;
};

template <typename T>
concept HasGetWorld = requires(std::shared_ptr<T> t) {
	{ t->GetWorld() } -> std::same_as<World::Ptr>;
};

export class LoggerPrint : public System
{
	
private:
	inline static std::shared_ptr<LoggerPrint> PInstanceLoggerPrint;

	inline static ELogLevel OELogLevel = ELogLevel_Debug;

	inline static std::ofstream OPidLogFile;


protected:
	
	friend class UniversalMemoryPool;
	LoggerPrint(World::WPtr world):System(world),
	AddLogFile(this)
	{
		emSystemType = EMSystemType::LoggerPrint;
	}

public:
	using Ptr = std::shared_ptr<LoggerPrint>;
	using CVPtr = const Ptr&;
	virtual ~LoggerPrint()
	{
		
	}

	virtual void Dispose()
	{
		System::Dispose();

		PInstanceLoggerPrint = nullptr;
	}

	/// @brief set logger type and Log file Init 
	void SetLoggerLevel(ELogLevel level)
	{
		OELogLevel = level;
	}

	bool Awake() override
	{
		PInstanceLoggerPrint = GetSelf<LoggerPrint>();

		World::CVPtr world = GetWorld();
		std::string* value = world->LaunchParam("LoggerLevel");
		if(!value)
		{
			return false;
		}

		ELogLevel logLevel = ELogLevel_Debug;
		std::string strType = "ELogLevel_" + *value;
		ELogLevel_Parse(strType, &logLevel);
		SetLoggerLevel(logLevel);

		std::filesystem::path path = *world->LaunchParam("PidLogFolder");
		if(!std::filesystem::exists(path))
		{
			std::filesystem::create_directories(path);
		}
		OPidLogFile = std::ofstream( std::format("{}/Output.log", path.string()), std::ios::app);
		
		return true;
	}

	template <typename... Args>
    void Record(World::Ptr world, ELogLevel level, const std::format_string<Args...>& fmt, Args&&... args)
	{
		if (level < OELogLevel)
		{
			return;
		}

		flush(world, level, std::format(fmt, std::forward<Args>(args)...));
	}

	template <typename... Args>
	void Record(World::Ptr world, EL10nCode code, Args&&... args)
	{
		ELogLevel level = ELogLevel_None;
		const std::string& fmt = GetL10nText()->GetTipText(code, level);

		if (level < OELogLevel)
		{
			return;
		}
	
		flush(world, level, std::vformat(fmt, std::make_format_args(args...)));
	}

	L10nText::Ptr GetL10nText() { return pL10nText.expired() ? nullptr : pL10nText.lock(); }

	void SetL10nText(L10nText::WPtr l10n){ pL10nText = l10n; }

public:

	template <HasGetWorld T, typename... Args>
    static void Log(std::shared_ptr<T> owner, ELogLevel level, const std::format_string<Args...>& fmt, Args&&... args)
	{
		Log(owner->GetWorld(), level, fmt, std::forward<Args>(args)...);
	}

	template <HasGetWorld T, typename... Args>
	static void Log(std::shared_ptr<T> owner, EL10nCode code, Args&&... args)
	{
		Log(owner->GetWorld(), code, std::forward<Args>(args)...);
	}

	template <typename... Args>
    static void Log(World::Ptr world, ELogLevel level, const std::format_string<Args...>& fmt, Args&&... args)
	{
		GetInstance()->Record(world, level, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
    static void Log(World::Ptr world, EL10nCode code, Args&&... args)
	{
		GetInstance()->Record(world, code, std::forward<Args>(args)...);
	}

protected:

	static LoggerPrint::Ptr GetInstance()
	{
		if(!PInstanceLoggerPrint)
		{
			if(P_InstanceHolder->AuthWorld)
			{
				PInstanceLoggerPrint = P_InstanceHolder->AuthWorld->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint);
			}
		}

		return PInstanceLoggerPrint;
	}

protected:

	void flush(World::Ptr world, ELogLevel level, std::string result)
	{
		if (result.empty())
		{
			return;
		}

		std::string* sTitle = nullptr;
		if(world)
		{
			sTitle = world->LaunchParam("svrName");
		}

		switch (level)
		{
			case ELogLevel_Normal:
				result = std::format("[{}] {} -> \n\t{}{}{}\n", GetNowTimeStr(), sTitle ? *sTitle : "", // olocation.function_name(),
					LogColor::BLUE, result, LogColor::RESET);
				break;
			case ELogLevel_Warning:
				result = std::format("[{}] {} -> \n\t{}{}{}\n", GetNowTimeStr(), sTitle ? *sTitle : "", // olocation.function_name(),
					LogColor::YELLOW, result, LogColor::RESET);
				break;
			case ELogLevel_Error:
				result = std::format("[{}] {} -> \n\t{}{}{}\n", GetNowTimeStr(), sTitle ? *sTitle : "", // olocation.function_name(),
					LogColor::RED, result, LogColor::RESET);
				break;
			case ELogLevel_Debug:
				result = std::format("[{}] {} -> \n\t{}\n", GetNowTimeStr(), sTitle ? *sTitle : "", // olocation.function_name(),
					result);
					break;
			default:
				return;
		}

		std::cout << result;

		LogToFile(sTitle, result);
	}

	void LogToFile(const std::string* serverName, const std::string& logMessage)
	{
		if(serverName)
		{
			std::shared_ptr<std::ofstream> logFile;
			if(mLogFileMap.count(*serverName) == 0)
			{
				std::filesystem::path path = *GetWorld()->LaunchParam("PidLogFolder");
				path /= std::format("{}.log", *serverName);

				logFile = AddLogFile(path.string(), std::ios::app);
			}
			else
			{
				logFile = mLogFileMap[*serverName];
			}
			
			
			*logFile << logMessage;
			logFile->flush();
			return;
		}

		if (OPidLogFile.is_open())
		{
			OPidLogFile << logMessage;
			OPidLogFile.flush();
		}
	}

	std::shared_ptr<std::ofstream> _AddLogFile(const std::string& filePath, const std::ios_base::openmode& mode)
	{
		std::shared_ptr<std::ofstream> logFile = std::make_shared<std::ofstream>(filePath, mode);

		mLogFileMap.emplace(filePath, logFile);
		
		return logFile;
	}

public:

	FunctionContainer<&LoggerPrint::_AddLogFile> AddLogFile;

protected:
	L10nText::WPtr pL10nText;

	std::filesystem::path oLogFolderPath;

	std::unordered_map<std::string, std::shared_ptr<std::ofstream>> mLogFileMap;
};

bool L10nText::Awake()
{
	World::CVPtr world = GetWorld();

	LoggerPrint::CVPtr pLogger = world->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint);
	std::string* value = world->LaunchParam("l10nDataPath");
	if (!value)
	{
		LoggerPrint::Log(nullptr, ELogLevel_Error, "Launch Param l10nErrPath Error !");
		return false;
	}

	mL10nCode.Clear();
	std::ifstream input(*value, std::ios::in | std::ios::binary);
	if (!input || !mL10nCode.ParseFromIstream(&input))
	{
		LoggerPrint::Log(nullptr, ELogLevel_Error, "load I10n Tip Config Error !");
		return false;
	}

	eType = EL10nType_zh_CN;
	if (value = world->LaunchParam("l10nLang"))
	{	
		std::string strType = "EL10nType_" + *value;
		if(!EL10nType_Parse(strType, &eType))
		{
			LoggerPrint::Log(nullptr, ELogLevel_Error, "load I10n l10nLang Error !");
			return false;
		}
	}

	switch (eType)
	{
		case EL10nType_zh_CN:
		{
			pL10nTipFunc = &l10n::l10nCode::zhcn;
			break;
		}
		case EL10nType_en_US:
		{
			pL10nTipFunc = &l10n::l10nCode::enus;
			break;
		}
		default:
			LoggerPrint::Log(nullptr, ELogLevel_Error, "load I10n Lang Type Error !");
			return false;
	}

	pLogger->SetL10nText(GetSelfW<L10nText>());

	return true;
}
