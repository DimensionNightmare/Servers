module;
export module L10nText;

import Config.Server;
import ThirdParty.PbGen;
import StrUtils;
import DllUtils;
import ECSW;

#define FUNCPLACE(func) #func, func

export class DNl10n : public System
{
protected:
	DNl10n(std::shared_ptr<World> world) 
		: System(world)
	{
		eSystemType = EMSystemType::DNl10n;
	}

	friend class World;
public:
	using Ptr = std::shared_ptr<DNl10n>;
	
	virtual ~DNl10n()
	{
		mL10nCodeDll.clear();
	}

	bool Awake() override
	{
		// if (const char* codeStr = Init())
		// {
		// 	// SPidLogger.Record(ELogLevel_Error, codeStr);
		// 	return false;
		// }

		return true;
	}

	/// PB's map find key need same runtimespace.
	/// reason is absl hashkey need random address.
	/// absl\hash\internal\hash.h kSeed
	const char* Init()
	{
		std::string* value = GetWorld()->LuanchParam("l10nDataPath");
		if (!value)
		{
			// LoggerPrint(ELogLevel_Debug)( "Launch Param l10nErrPath Error !");
			return "Launch Param l10nErrPath Error !";
		}

		mL10nCode.Clear();
		std::ifstream input(*value, std::ios::in | std::ios::binary);
		if (!input || !mL10nCode.ParseFromIstream(&input))
		{
			// LoggerPrint(ELogLevel_Debug)( "load I10n Tip Config Error !");
			return "load I10n Tip Config Error !";
		}

		mL10nCodeDll.clear();

		for (auto& one : mL10nCode.data_map())
		{
			mL10nCodeDll[one.first] = &one.second;
		}
		
		eType = EL10nType_zh_CN;
		value =  GetWorld()->LuanchParam("l10nLang");
		if (!value && !EL10nType_Parse(*value, &eType))
		{	
			
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
				// LoggerPrint(ELogLevel_Debug)( "load I10n Lang Type Error !");
				return "load I10n Lang Type Error !";
		}

		return nullptr;
	}

public:

	/// @brief main use this
	l10n::l10nCodes mL10nCode;

	/// @brief dll use this
	std::unordered_map<uint32_t, const l10n::l10nCode*> mL10nCodeDll;

	/// @brief l10n imp. text get.
	typedef const std::string& (l10n::l10nCode::* TipTextFunc)() const;
	TipTextFunc pL10nTipFunc = nullptr;

	/// @brief l10n type
	EL10nType eType = EL10nType_zh_CN;

public:

 	static const std::string& GetTipText(EL10nCode type, ELogLevel& logLevel)
	{
		logLevel = ELogLevel_None;
		
		static DNl10n* instance = PInstance ? PInstance.get() : GetDllInstance();
		if (!instance)
		{
			throw std::runtime_error(std::format("I10n GetTipText Cant Get Instance!"));
		}

		auto& dataMap = instance->mL10nCodeDll;
		if (!dataMap.contains(type))
		{
			throw std::invalid_argument(std::format("I10n Tip Config not exist this type {}", PbGen::EL10nCode_Name_(type)));
		}

		auto& one = dataMap[type];

		logLevel = one->level();
		return (one->*(instance->pL10nTipFunc))();
	}

	DNl10n* GetInstance()
	{
		return PInstance.get();
	}

	static DNl10n* GetDllInstance()
	{
		return TickMainSpaceDll(PInstance.get(), FUNCPLACE(&DNl10n::GetInstance));
	}

public:
	static inline std::shared_ptr<DNl10n> PInstance = nullptr;
};
