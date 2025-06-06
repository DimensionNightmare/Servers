module;
export module L10nText;

import ECSW;
import ThirdParty.Protobuf;

#define FUNCPLACE(class, func) &class::func, #class"_"#func

export class DNl10n : public System
{
protected:
	DNl10n(World::WPtr world) 
		: System(world)
	{
		emSystemType = EMSystemType::DNl10n;
	}

	friend class World;
public:
	using Ptr = std::shared_ptr<DNl10n>;
	using WPtr = std::weak_ptr<DNl10n>;
	
	virtual ~DNl10n()
	{
		
	}

	virtual void Dispose() override
	{
		System::Dispose();
		mL10nCodeDll.clear();
	}

	/// PB's map find key need same runtimespace.
	/// reason is absl hashkey need random address.
	/// absl\hash\internal\hash.h kSeed
	bool Init();

	const std::string& GetTipText(EL10nCode type, ELogLevel& logLevel)
	{
		logLevel = ELogLevel_None;
		
		if (!mL10nCodeDll.contains(type))
		{
			throw std::invalid_argument(std::format("I10n Tip Config not exist this type {}", PbGen::EL10nCode_Name_(type)));
		}

		auto& one = mL10nCodeDll[type];

		logLevel = one->level();
		return (one->*(pL10nTipFunc))();
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
};
