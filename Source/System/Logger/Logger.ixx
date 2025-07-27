module;
export module Logger;

import StrUtils;
import L10nText;
import ECSW;
import ThirdParty.Protobuf;
import std.compat;

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
	LoggerPrint(World::WPtr world):System(world)
	{
		emSystemType = EMSystemType::LoggerPrint;
	}

public:
	using Ptr = std::shared_ptr<LoggerPrint>;
	using CVPtr = const Ptr&;
	using WPtr = std::weak_ptr<LoggerPrint>;
	~LoggerPrint()
	{

	}

	virtual void Dispose() override
	{
		System::Dispose();
	}

	bool Init()
	{
		World::CVPtr pWorld = GetWorld();
		std::string* value = pWorld->LaunchParam("LoggerLevel");
		if(!value)
		{
			return false;
		}

		ELogLevel logLevel = ELogLevel_Debug;
		std::string strType = "ELogLevel_" + *value;
		ELogLevel_Parse(strType, &logLevel);
		SetLogger(logLevel);

		value = pWorld->LaunchParam("program");
		if(!value)
		{
			return false;
		}
		
		std::filesystem::path logFile = *value;

		value = pWorld->LaunchParam("svrName");

		logFile = logFile.parent_path() / *value;
		SetLogger(logFile);

		sTitle = *value;

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
		ELogLevel level;
		const std::string& fmt = Getl10nText()->GetTipText(code, level);

		if (level < eLogLevel)
		{
			return;
		}

		// std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
		auto&& args_tuple = std::forward_as_tuple(std::forward<Args>(args)...);

        auto format_args = std::apply([](auto&&... args) {
            return std::make_format_args(args...);
        }, args_tuple);
		
	
		flush(level, std::vformat(fmt, format_args));
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

	l10nText::Ptr Getl10nText() { return pl10nText.expired() ? nullptr : pl10nText.lock(); }

	void Setl10nText(l10nText::WPtr l10n){ pl10nText = l10n; }

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
	l10nText::WPtr pl10nText;

	std::ofstream LogFile; 

	std::string sTitle;

	ELogLevel eLogLevel = ELogLevel_Debug;
};

bool l10nText::Init()
{
	World::CVPtr pWorld = GetWorld();

	LoggerPrint::CVPtr pLogger = pWorld->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint);
	std::string* value = pWorld->LaunchParam("l10nDataPath");
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
	if (value = pWorld->LaunchParam("l10nLang"))
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
			pL10nTipFunc = &l10n::l10nCode::zh_cn;
			break;
		}
		case EL10nType_en_US:
		{
			pL10nTipFunc = &l10n::l10nCode::en_us;
			break;
		}
		default:
			pLogger->Record(ELogLevel_Error, "load I10n Lang Type Error !");
			return false;
	}

	pLogger->Setl10nText(GetSelfW<l10nText>());

	return true;
}

class LoggerPrintPid
{
public:
	LoggerPrintPid()
	{
		pWorld = std::make_shared<World>();
		pWorld->AddSystem<l10nText>();
		pLogger = pWorld->AddSystem<LoggerPrint>();
	}

	~LoggerPrintPid()
	{
		pWorld->Dispose();
		pWorld = nullptr;
	}

	bool Init(ELogLevel level)
	{
		GetLogger()->SetLogger(level);
		return true;
	}

	bool Init(const std::filesystem::path& path)
	{
		GetLogger()->SetLogger(path);
		return true;
	}

	bool Init(std::unordered_map<std::string, std::string> commonInfo)
	{
		pWorld->MoveLuanchConfigToSelf(std::move(commonInfo));
		l10nText::CVPtr pL10n = pWorld->GetSystem<l10nText>(EMSystemType::l10nText);
		return pL10n->Init();
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

private:
	World::Ptr pWorld;
	LoggerPrint::WPtr pLogger;
};

export LoggerPrintPid SPidLogger;
