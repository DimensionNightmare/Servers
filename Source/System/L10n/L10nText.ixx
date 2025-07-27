module;
export module L10nText;

import ECSW;
import ThirdParty.Protobuf;
import std.compat;

#define FUNCPLACE(class, func) &class::func, #class"_"#func

export class L10nText : public System
{
protected:
	L10nText(World::WPtr world)
		: System(world)
	{
		emSystemType = EMSystemType::L10nText;

		// create code space ..0.0..
		pPBMapFindFunc = [this](EL10nCode type, ELogLevel& logLevel)->const std::string&
			{
				auto& map = mL10nCode.data_map();
				auto finder = map.find(type);
				if (finder == map.end())
				{
					throw std::invalid_argument(std::format("I10n Tip Config not exist this type {}", PbGen::EL10nCode_Name_(type)));
				}
				
				logLevel = finder->second.level();
				return std::invoke(pL10nTipFunc, &finder->second);
			};
	}

	friend class World;
public:
	using Ptr = std::shared_ptr<L10nText>;
	using CVPtr = const Ptr&;
	using WPtr = std::weak_ptr<L10nText>;

	virtual ~L10nText()
	{

	}

	virtual void Dispose() override
	{
		System::Dispose();
	}

	/// PB's map find key need same runtimespace.
	/// reason is absl hashkey need random address.
	/// absl\hash\internal\hash.h kSeed
	bool Init();

	const std::string& GetTipText(EL10nCode type, ELogLevel& logLevel)
	{
		logLevel = ELogLevel_None;

		// auto finded = pPBMapFindFunc(mL10nCode.data_map(), type);

		// logLevel = finded->second.level();
		// // return ((finded->second).*(pL10nTipFunc))();
		// return std::invoke(pL10nTipFunc, &finded->second);
		return pPBMapFindFunc(type, logLevel);
	}

public:

	/// @brief main use this
	l10n::l10nCodes mL10nCode;

	/// @brief l10n imp. find get.
	// FindFunctionPtr pPBMapFindFunc = nullptr;
	std::function<const std::string&(EL10nCode, ELogLevel&)> pPBMapFindFunc = nullptr;

	/// @brief l10n imp. text get.
	typedef const std::string& (l10n::l10nCode::* TipTextFunc)() const;
	TipTextFunc pL10nTipFunc = nullptr;

	/// @brief l10n type
	EL10nType eType = EL10nType_zh_CN;
};
