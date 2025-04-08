module;
#include "StdMacro.h"
export module L10nText;

import Config.Server;
import ThirdParty.PbGen;
import StrUtils;
import DllUtils;

export class DNl10n
{
	
public:
	
	DNl10n()
	{
		
	}
	
	virtual ~DNl10n()
	{
		mL10nCodeDll.clear();
	}

	/// PB's map find key need same runtimespace.
	/// reason is absl hashkey need random address.
	/// absl\hash\internal\hash.h kSeed
	const char* Init()
	{
		std::string* value = LaunchConfig::GetParam("l10nDataPath");
		if (!value)
		{
			// DNPrint(ELogLevel_Debug, "Launch Param l10nErrPath Error !");
			return "Launch Param l10nErrPath Error !";
		}

		mL10nCode.Clear();
		std::ifstream input(*value, std::ios::in | std::ios::binary);
		if (!input || !mL10nCode.ParseFromIstream(&input))
		{
			// DNPrint(ELogLevel_Debug, "load I10n Tip Config Error !");
			return "load I10n Tip Config Error !";
		}

		mL10nCodeDll.clear();

		for (auto& one : mL10nCode.data_map())
		{
			mL10nCodeDll[one.first] = &one.second;
		}
		
		eType = EL10nType_zh_CN;
		value = LaunchConfig::GetParam("l10nLang");
		if (!value && !Common::EL10nType_Parse(*value, &eType))
		{	
			
		}

		switch (eType)
		{
			case EL10nType_zh_CN:
			{
				pL10nTipFunc = &l10nCode::zh_cn;
				break;
			}
			case EL10nType_en_US:
			{
				pL10nTipFunc = &l10nCode::en_us;
				break;
			}
			default:
				// DNPrint(ELogLevel_Debug, "load I10n Lang Type Error !");
				return "load I10n Lang Type Error !";
		}

		return nullptr;
	}

public:

	/// @brief main use this
	l10nCodes mL10nCode;

	/// @brief dll use this
	std::unordered_map<uint32_t, const l10nCode*> mL10nCodeDll;

	/// @brief l10n imp. text get.
	typedef const std::string& (l10nCode::* TipTextFunc)() const;
	TipTextFunc pL10nTipFunc = nullptr;

	/// @brief l10n type
	EL10nType eType = EL10nType_zh_CN;

public:

 	static void GetTipText(EL10nCode type, ELogLevel& logLevel, const char*& fmt)
	{
		logLevel = ELogLevel_None;
		fmt = nullptr;

		static std::shared_ptr<DNl10n> instance = PInstance ? PInstance : GetDllInstance();
		if (!instance)
		{
			return;
		}

		auto& dataMap = instance->mL10nCodeDll;
		if (!dataMap.contains(type))
		{
			throw std::invalid_argument(std::format("I10n Tip Config not exist this type {}", Common::EL10nCode_Name(type)));
		}

		auto& one = dataMap[type];

		logLevel = one->level();
		fmt = (one->*(instance->pL10nTipFunc))().c_str();
	}

	DNl10n* GetInstance()
	{
		return PInstance.get();
	}

	static std::shared_ptr<DNl10n> GetDllInstance()
	{

		if(DNl10n* handle = TICK_MAINSPACE_SIGN_FUNCTION(DNl10n, GetInstance, PInstance.get()))
		{
			return std::shared_ptr<DNl10n>(handle, [](DNl10n* obj){});
		}
		return nullptr;
	}

public:
	static inline std::shared_ptr<DNl10n> PInstance = nullptr;
};
