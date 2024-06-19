module;
#include <fstream>
#include <format>
#include <string>
#include <iostream>
#include <unordered_map>

export module I10nText;

import Config.Server;
import ThirdParty.PbGen;

using namespace std;

enum class LangType : uint8_t
{
	zh_CN,
	en_US,
};

export class DNl10n
{
public:
	DNl10n();
	~DNl10n();
	const char* InitConfigData();
public:
	L10nErrs mL10nErr;
	unordered_map<uint32_t, const l10nErr*> mL10nErrDll;

	L10nTips mL10nTip;
	unordered_map<uint32_t, const l10nTip*> mL10nTipDll;

	typedef const string& (l10nErr::* ErrTextFunc)() const;
	ErrTextFunc pL10nErrFunc = nullptr;

	typedef const string& (l10nTip::* TipTextFunc)() const;
	TipTextFunc pL10nTipFunc = nullptr;

	LangType eType = LangType::zh_CN;
};

DNl10n::DNl10n()
{
}

DNl10n::~DNl10n()
{
	mL10nErrDll.clear();
	mL10nTipDll.clear();

}

DNl10n* PInstance = nullptr;
bool DllSpace = false;

export void SetDNl10nInstance(DNl10n* point, bool IsDllInit = false)
{
	PInstance = point;

	DllSpace = IsDllInit;

	/// PB's map find key need same runtimespace.
	/// reason is absl hashkey need random address.
	/// absl\hash\internal\hash.h kSeed
	if(DllSpace)
	{
		auto& errMap = PInstance->mL10nErrDll;
		errMap.clear();

		for (auto& one : PInstance->mL10nErr.data_map())
		{
			errMap[one.first] = &one.second;
		}

		auto& tipMap = PInstance->mL10nTipDll;
		tipMap.clear();

		for (auto& one : PInstance->mL10nTip.data_map())
		{
			tipMap[one.first] = &one.second;
		}		
	}
}

const char* DNl10n::InitConfigData()
{
	string* value = GetLuanchConfigParam("l10nErrPath");
	if (!value)
	{
		// DNPrint(0, LoggerLevel::Debug, "Launch Param l10nErrPath Error !");
		return "Launch Param l10nErrPath Error !";
	}

	{
		mL10nErr.Clear();
		ifstream input(*value, ios::in | ios::binary);
		if (!input || !mL10nErr.ParseFromIstream(&input))
		{
			// DNPrint(0, LoggerLevel::Debug, "load I10n Err Config Error !");
			return "load I10n Err Config Error !";
		}
	}

	value = GetLuanchConfigParam("l10nTipPath");
	if (!value)
	{
		// DNPrint(0, LoggerLevel::Debug, "Launch Param l10nTipPath Error Error !");
		return "Launch Param l10nTipPath Error Error !";
	}

	{
		mL10nTip.Clear();
		ifstream input(*value, ios::in | ios::binary);
		if (!input || !mL10nTip.ParseFromIstream(&input))
		{
			// DNPrint(0, LoggerLevel::Debug, "load I10n Tip Config Error !");
			return "load I10n Tip Config Error !";
		}
	}

	value = GetLuanchConfigParam("l10nLang");
	if (value)
	{
		eType = (LangType)stoi(*value);
	}

	switch (eType)
	{
		case LangType::zh_CN:
		{
			pL10nErrFunc = &l10nErr::zh;
			pL10nTipFunc = &l10nTip::zh;
			break;
		}
		case LangType::en_US:
		{
			pL10nErrFunc = &l10nErr::en;
			pL10nTipFunc = &l10nTip::en;
			break;
		}
		default:
			// DNPrint(0, LoggerLevel::Debug, "load I10n Lang Type Error !");
			return "load I10n Lang Type Error !";
	}

	SetDNl10nInstance(this);

	return nullptr;
}

export const char* GetErrText(int type)
{
	if (!PInstance || !PB_ErrCode_IsValid(type))
	{
		return nullptr;
	}

	const l10nErr *finded = nullptr;

	if(!DllSpace)
	{
		auto& dataMap = PInstance->mL10nErr.data_map();
		auto data = dataMap.find(type);
		if (data == dataMap.end())
		{
			throw invalid_argument(format("I10n Err Config not exist this type {}", PB_ErrCode_Name(type)));
		}

		finded = &data->second;
	}
	else
	{
		auto& dataMap = PInstance->mL10nErrDll;
		auto data = dataMap.find(type);
		if (data == dataMap.end())
		{
			throw invalid_argument(format("I10n Err Config not exist this type {}", PB_ErrCode_Name(type)));
		}

		finded = data->second;
	}
	
	return (finded->*(PInstance->pL10nErrFunc))().c_str();
}

export const char* GetTipText(int type)
{
	if (!PInstance || !PB_TipCode_IsValid(type))
	{
		return nullptr;
	}

	const l10nTip *finded = nullptr;

	if(!DllSpace)
	{
		auto& dataMap = PInstance->mL10nTip.data_map();
		auto data = dataMap.find(type);
		if (data == dataMap.end())
		{
			throw invalid_argument(format("I10n Tip Config not exist this type {}", PB_TipCode_Name(type)));
		}

		finded = &data->second;
	}
	else
	{
		auto& dataMap = PInstance->mL10nTipDll;
		auto data = dataMap.find(type);
		if (data == dataMap.end())
		{
			throw invalid_argument(format("I10n Tip Config not exist this type {}", PB_TipCode_Name(type)));
		}

		finded = data->second;
	}

	return (finded->*(PInstance->pL10nTipFunc))().c_str();
}