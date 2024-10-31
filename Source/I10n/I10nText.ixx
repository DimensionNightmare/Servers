module;
#include "StdMacro.h"
export module I10nText;

import Config.Server;
import ThirdParty.PbGen;

enum class EMLangType : uint8_t
{
	zh_CN,
	en_US,
};

class DNl10n;

bool DllSpace = false;

DNl10n* PInstance = nullptr;

/// @brief same serverconfig
export void SetDNl10nInstance(DNl10n* point)
{
	PInstance = point;
}

export class DNl10n
{
	
public:
	
	DNl10n(){}
	
	~DNl10n()
	{
		mL10nErrDll.clear();
		mL10nTipDll.clear();
	}

	/// PB's map find key need same runtimespace.
	/// reason is absl hashkey need random address.
	/// absl\hash\internal\hash.h kSeed
	const char* InitConfigData()
	{
		string* value = GetLuanchConfigParam("l10nErrPath");
		if (!value)
		{
			// DNPrint(0, EMLoggerLevel::Debug, "Launch Param l10nErrPath Error !");
			return "Launch Param l10nErrPath Error !";
		}

		{
			mL10nErr.Clear();
			ifstream input(*value, ios::in | ios::binary);
			if (!input || !mL10nErr.ParseFromIstream(&input))
			{
				// DNPrint(0, EMLoggerLevel::Debug, "load I10n Err Config Error !");
				return "load I10n Err Config Error !";
			}

			mL10nErrDll.clear();

			for (auto& one : mL10nErr.data_map())
			{
				mL10nErrDll[one.first] = &one.second;
			}

		}

		value = GetLuanchConfigParam("l10nTipPath");
		if (!value)
		{
			// DNPrint(0, EMLoggerLevel::Debug, "Launch Param l10nTipPath Error Error !");
			return "Launch Param l10nTipPath Error Error !";
		}

		{
			mL10nTip.Clear();
			ifstream input(*value, ios::in | ios::binary);
			if (!input || !mL10nTip.ParseFromIstream(&input))
			{
				// DNPrint(0, EMLoggerLevel::Debug, "load I10n Tip Config Error !");
				return "load I10n Tip Config Error !";
			}

			mL10nTipDll.clear();

			for (auto& one : mL10nTip.data_map())
			{
				mL10nTipDll[one.first] = &one.second;
			}
		}

		value = GetLuanchConfigParam("l10nLang");
		if (value)
		{
			eType = (EMLangType)stoi(*value);
		}

		switch (eType)
		{
			case EMLangType::zh_CN:
			{
				pL10nErrFunc = &l10nErr::zh;
				pL10nTipFunc = &l10nTip::zh;
				break;
			}
			case EMLangType::en_US:
			{
				pL10nErrFunc = &l10nErr::en;
				pL10nTipFunc = &l10nTip::en;
				break;
			}
			default:
				// DNPrint(0, EMLoggerLevel::Debug, "load I10n Lang Type Error !");
				return "load I10n Lang Type Error !";
		}

		SetDNl10nInstance(this);

		return nullptr;
	}

public:
	/// @brief main use this
	l10nErrs mL10nErr;
	/// @brief dll use this
	unordered_map<uint32_t, const l10nErr*> mL10nErrDll;
	/// @brief main use this
	l10nTips mL10nTip;
	/// @brief dll use this
	unordered_map<uint32_t, const l10nTip*> mL10nTipDll;

	/// @brief l10n imp. text get.
	typedef const string& (l10nErr::* ErrTextFunc)() const;
	ErrTextFunc pL10nErrFunc = nullptr;

	/// @brief l10n imp. text get.
	typedef const string& (l10nTip::* TipTextFunc)() const;
	TipTextFunc pL10nTipFunc = nullptr;

	/// @brief l10n type
	EMLangType eType = EMLangType::zh_CN;
	
};

export const char* GetErrText(int type)
{
	if (!PInstance || !PBExport::ErrCode_IsValid(type))
	{
		return nullptr;
	}

	auto& dataMap = PInstance->mL10nErrDll;
	if (!dataMap.contains(type))
	{
		throw invalid_argument(format("I10n Err Config not exist this type {}", PBExport::ErrCode_Name(type)));
	}

	return (dataMap[type]->*(PInstance->pL10nErrFunc))().c_str();
}

export const char* GetTipText(int type)
{
	if (!PInstance || !PBExport::TipCode_IsValid(type))
	{
		return nullptr;
	}

	auto& dataMap = PInstance->mL10nTipDll;
	if (!dataMap.contains(type))
	{
		throw invalid_argument(format("I10n Tip Config not exist this type {}", PBExport::TipCode_Name(type)));
	}

	return (dataMap[type]->*(PInstance->pL10nTipFunc))().c_str();
}
