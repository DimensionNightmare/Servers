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

export class LoggerPrint : public System
{
protected:
	
	friend class World;
	friend class UniversalMemoryPool;
	LoggerPrint(World::WPtr world):System(world)
	{
		emSystemType = EMSystemType::LoggerPrint;
	}

public:
	using Ptr = std::shared_ptr<LoggerPrint>;
	using CVPtr = const Ptr&;
	using WPtr = std::weak_ptr<LoggerPrint>;
	virtual ~LoggerPrint()
	{

	}

	virtual void Dispose() override
	{
		System::Dispose();
	}

	bool Awake() override
	{
		World::CVPtr world = GetWorld();
		std::string* value = world->LaunchParam("LoggerLevel");
		if(!value)
		{
			return false;
		}

		ELogLevel logLevel = ELogLevel_Debug;
		std::string strType = "ELogLevel_" + *value;
		ELogLevel_Parse(strType, &logLevel);
		SetLogger(logLevel);

		value = world->LaunchParam("program");
		if(!value)
		{
			return false;
		}
		
		std::filesystem::path logFile = *value;

		if(value = world->LaunchParam("svrName"))
		{
			logFile = logFile.parent_path() / *value;
			sTitle = *value;
		}

		SetLogger(logFile);

		return true;
	}

	template <typename... Args>
    void Record(ELogLevel level, const std::format_string<Args...>& fmt, Args&&... args)
	{
		if (level < eLogLevel)
		{
			return;
		}

		flush(level, std::format(fmt, std::forward<Args>(args)...));
	}


	template <typename... Args>
	void Record(EL10nCode code, Args&&... args)
	{
		ELogLevel level = ELogLevel_None;
		const std::string& fmt = GetL10nText()->GetTipText(code, level);

		if (level < eLogLevel)
		{
			return;
		}

		// std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
		// auto&& args_tuple = std::forward_as_tuple(std::forward<Args>(args)...);

        // auto format_args = std::apply([](auto&&... args) {
        //     return std::make_format_args(args...);
        // }, args_tuple);
		
	
		flush(level, std::vformat(fmt, std::make_format_args(args...)));
	}

	/// @brief set logger type and Log file Init 
	void SetLogger(ELogLevel level)
	{
		eLogLevel = level;
	}

	void SetLogger(const std::filesystem::path& path)
	{
		if(!LogFile.is_open())
		{
			if(!std::filesystem::exists(path))
			{
				std::filesystem::create_directories(path);
			}
			LogFile = std::ofstream( std::format("{}/Output.log", path.string()), std::ios::app);
		}
	}

	L10nText::Ptr GetL10nText() { return pL10nText.expired() ? nullptr : pL10nText.lock(); }

	void SetL10nText(L10nText::WPtr l10n){ pL10nText = l10n; }

protected:

	
	void flush(ELogLevel level, std::string result)
	{
		if (result.empty())
		{
			return;
		}

		switch (level)
		{
			case ELogLevel_Normal:
				result = std::format("[{}] {} -> \n\t{}{}{}\n", GetNowTimeStr(), sTitle, // olocation.function_name(),
					LogColor::BLUE, result, LogColor::RESET);
				break;
			case ELogLevel_Warning:
				result = std::format("[{}] {} -> \n\t{}{}{}\n", GetNowTimeStr(), sTitle, // olocation.function_name(),
					LogColor::YELLOW, result, LogColor::RESET);
				break;
			case ELogLevel_Error:
				result = std::format("[{}] {} -> \n\t{}{}{}\n", GetNowTimeStr(), sTitle, // olocation.function_name(),
					LogColor::RED, result, LogColor::RESET);
				break;
			case ELogLevel_Debug:
				result = std::format("[{}] {} -> \n\t{}\n", GetNowTimeStr(), sTitle, // olocation.function_name(),
					result);
					break;
			default:
				return;
		}

		std::cout << result;
				
		if (LogFile.is_open())
		{
			LogFile << result;
			LogFile.flush();
		}
	}


protected:
	L10nText::WPtr pL10nText;

	std::ofstream LogFile; 

	std::string sTitle;

	ELogLevel eLogLevel = ELogLevel_Debug;
};

bool L10nText::Awake()
{
	World::CVPtr world = GetWorld();

	LoggerPrint::CVPtr pLogger = world->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint);
	std::string* value = world->LaunchParam("l10nDataPath");
	if (!value)
	{
		pLogger->Record(ELogLevel_Error, "Launch Param l10nErrPath Error !");
		return false;
	}

	mL10nCode.Clear();
	std::ifstream input(*value, std::ios::in | std::ios::binary);
	if (!input || !mL10nCode.ParseFromIstream(&input))
	{
		pLogger->Record(ELogLevel_Error, "load I10n Tip Config Error !");
		return false;
	}

	eType = EL10nType_zh_CN;
	if (value = world->LaunchParam("l10nLang"))
	{	
		std::string strType = "EL10nType_" + *value;
		if(!EL10nType_Parse(strType, &eType))
		{
			pLogger->Record(ELogLevel_Error, "load I10n l10nLang Error !");
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
			pLogger->Record(ELogLevel_Error, "load I10n Lang Type Error !");
			return false;
	}

	pLogger->SetL10nText(GetSelfW<L10nText>());

	return true;
}

export class LoggerPrintPid
{
public:
	LoggerPrintPid()
	{
		pWorld = MemPool->Allocate<World>();

		std::filesystem::path exePath = Platform::GetExecutablePath();
		
		oPidWorkPath = exePath.parent_path() / std::format("PID_LOG/PID_{}", Platform::GetCurrentProcessId());

		std::unordered_map<std::string, std::string> commonInfo = {
			{"program", oPidWorkPath.string()},
			{"LoggerLevel", "Debug"},
		};

		pWorld->MoveLuanchConfigToSelf(commonInfo);

		pLogger = pWorld->AddSystem<LoggerPrint>();
	}

	~LoggerPrintPid()
	{
		if(pWorld)
		{
			pWorld->Dispose();
			pWorld = nullptr;
		}
	}

	bool Init(ELogLevel level)
	{
		GetLogger()->SetLogger(level);
		return true;
	}

	bool Init(std::unordered_map<std::string, std::string> commonInfo)
	{
		pWorld->MoveLuanchConfigToSelf(commonInfo);
		
		L10nText::CVPtr l10nText = pWorld->AddSystem<L10nText>();
		if(!l10nText)
		{
			return false;
		}
		return true;
	}

	template <typename... Args>
    void Record(ELogLevel level, const std::format_string<Args...>& fmt, Args&&... args)
	{
		GetLogger()->Record(level, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void Record(EL10nCode code, Args&&... args)
	{
		GetLogger()->Record(code, std::forward<Args>(args)...);
	}

	LoggerPrint::Ptr GetLogger() { return pLogger.expired() ? nullptr : pLogger.lock(); }

	const std::filesystem::path& GetPidWorkPath() const { return oPidWorkPath; }

private:
	World::Ptr pWorld;
	LoggerPrint::WPtr pLogger;
	std::filesystem::path oPidWorkPath;
};

export std::shared_ptr<LoggerPrintPid> SPidLogger; // dynamic initializer
