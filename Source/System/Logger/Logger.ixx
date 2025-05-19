module;
export module Logger;

import StrUtils;
import L10nText;
import ECSW;
export import ThirdParty.PbGen;

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
	LoggerPrint(World::WPtr world):System(world)
	{
		emSystemType = EMSystemType::LoggerPrint;
	}

public:
	using Ptr = std::shared_ptr<LoggerPrint>;
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
		World::Ptr pWorld = GetWorld();
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
		logFile = logFile.parent_path() / *pWorld->LaunchParam("svrName");
		SetLogger(logFile);

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
		const std::string& fmt = GetDNl10n()->GetTipText(code, level);

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
		const std::string& fmt = GetDNl10n()->GetTipText(code, level);

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

	DNl10n::Ptr GetDNl10n() { return pDNl10n.expired() ? nullptr : pDNl10n.lock(); }

	void SetDNl10n(DNl10n::WPtr l10n){ pDNl10n = l10n; }
	
protected:
	DNl10n::WPtr pDNl10n;

	std::ofstream LogFile; 

	ELogLevel eLogLevel = ELogLevel_Debug;
};

bool DNl10n::Init()
{
	World::Ptr pWorld = GetWorld();

	LoggerPrint::Ptr pLogger = pWorld->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint);
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

	mL10nCodeDll.clear();

	for (auto& one : mL10nCode.data_map())
	{
		mL10nCodeDll[one.first] = &one.second;
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

	pLogger->SetDNl10n(GetSelfW<DNl10n>());

	return true;
}

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
		DNl10n::Ptr pL10n = pWorld->GetSystem<DNl10n>(EMSystemType::DNl10n);
		return pL10n->Init();
	}

	template <typename... Args>
    void Record(ELogLevel level, const std::format_string<Args...>& fmt, Args&&... args)
	{
		GetLogger()->Record(level, fmt, std::forward<Args>(args)...);
	}

	void Record(ELogLevel level, const std::string& fmt)
	{
		GetLogger()->Record(level, fmt);
	}

	template <typename... Args>
	void Record(EL10nCode code, Args&&... args)
	{
		GetLogger()->Record(code, std::forward<Args>(args)...);
	}

	void Record(EL10nCode code)
	{
		GetLogger()->Record(code);
	}

	LoggerPrint::Ptr GetLogger() { return pLogger.expired() ? nullptr : pLogger.lock(); }

private:
	World::Ptr pWorld;
	LoggerPrint::WPtr pLogger;
};

export LoggerPrintPid SPidLogger;
