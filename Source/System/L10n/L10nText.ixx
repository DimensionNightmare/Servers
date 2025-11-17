export module L10nText;

import ECSW;
import ThirdParty.Protobuf;
import std.compat;
import FuncUtils;

export class L10nText : public System
{
protected:
	L10nText(World::WPtr world) : System(world)
		// create code space ..0.0..
		, GetTipText(this)
	{
		emSystemType = EMSystemType::L10nText;

	}

	friend class UniversalMemoryPool;
public:
	using Ptr = std::shared_ptr<L10nText>;
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
	bool Awake() override;

protected:

	const std::string& _GetTipText(EL10nCode type, ELogLevel& logLevel)
	{
		logLevel = ELogLevel_None;

		auto& map = mL10nCode.data_map();
		auto finder = map.find(type);
		if (finder == map.end())
		{
			throw std::invalid_argument(std::format("I10n Tip Config not exist this type {}", PbGen::EL10nCode_Name_(type)));
		}
		
		logLevel = finder->second.level();
		return std::invoke(pL10nTipFunc, &finder->second);
	}

public:

	/// @brief l10n imp. find get.
	// FindFunctionPtr pPBMapFindFunc = nullptr;
	EventContainer<&L10nText::_GetTipText> GetTipText;

private:

	/// @brief main use this
	l10n::l10nCodes mL10nCode;


	/// @brief l10n imp. text get.
	typedef const std::string& (l10n::l10nCode::* TipTextFunc)() const;
	TipTextFunc pL10nTipFunc = nullptr;

	/// @brief l10n type
	EL10nType eType = EL10nType_zh_CN;
};
