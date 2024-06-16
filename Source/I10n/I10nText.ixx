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
	typedef const string& (ErrText::* ErrTextFunc)() const;
	typedef const string& (TipText::* TipTextFunc)() const;
public:
	DNl10n();
	~DNl10n();
	const char* InitConfigData();
public:
	unique_ptr<L10nErr> pErrMsgData;
	unordered_map<uint32_t, const ErrText*> mErrMsgDllData;

	unique_ptr<L10nTip> pTipMsgData;
	unordered_map<uint32_t, const TipText*> mTipMsgDllData;

	ErrTextFunc pErrMsgFunc = nullptr;
	TipTextFunc pTipMsgFunc = nullptr;

	LangType eType = LangType::zh_CN;
};

DNl10n::DNl10n()
{
}

DNl10n::~DNl10n()
{
	pErrMsgData = nullptr;
	pTipMsgData =  nullptr;

	mErrMsgDllData.clear();
	mTipMsgDllData.clear();

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
		auto& errMap = PInstance->mErrMsgDllData;
		errMap.clear();

		for (auto& one : PInstance->pErrMsgData->data_map())
		{
			errMap[one.first] = &one.second;
		}

		auto& tipMap = PInstance->mTipMsgDllData;
		tipMap.clear();

		for (auto& one : PInstance->pTipMsgData->data_map())
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

	if (pErrMsgData)
	{
		pErrMsgData->Clear();
	}
	else
	{
		pErrMsgData = make_unique<L10nErr>();
	}

	{
		ifstream input(*value, ios::in | ios::binary);
		if (!input || !pErrMsgData->ParseFromIstream(&input))
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

	if (pTipMsgData)
	{
		pTipMsgData->Clear();
	}
	else
	{
		pTipMsgData = make_unique<L10nTip>();
	}

	{
		ifstream input(*value, ios::in | ios::binary);
		if (!input || !pTipMsgData->ParseFromIstream(&input))
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
			pErrMsgFunc = &ErrText::zh;
			pTipMsgFunc = &TipText::zh;
			break;
		}
		case LangType::en_US:
		{
			pErrMsgFunc = &ErrText::en;
			pTipMsgFunc = &TipText::en;
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

	if(DllSpace)
	{
		auto& dataMap = PInstance->pErrMsgData->data_map();
		auto data = dataMap.find(type);
		if (data == dataMap.end())
		{
			throw invalid_argument(format("I10n Err Config not exist this type {}", PB_ErrCode_Name(type)));
		}

		return (data->second.*(PInstance->pErrMsgFunc))().c_str();
	}
	else
	{
		auto& dataMap = PInstance->mErrMsgDllData;
		auto data = dataMap.find(type);
		if (data == dataMap.end())
		{
			throw invalid_argument(format("I10n Err Config not exist this type {}", PB_ErrCode_Name(type)));
		}

		return (data->second->*(PInstance->pErrMsgFunc))().c_str();
	}
	
}

export const char* GetTipText(int type)
{
	if (!PInstance || !PB_TipCode_IsValid(type))
	{
		return nullptr;
	}

	if(!DllSpace)
	{
		auto& dataMap = PInstance->pTipMsgData->data_map();
		auto data = dataMap.find(type);
		if (data == dataMap.end())
		{
			throw invalid_argument(format("I10n Tip Config not exist this type {}", PB_TipCode_Name(type)));
		}

		return (data->second.*(PInstance->pTipMsgFunc))().c_str();
	}
	else
	{
		auto& dataMap = PInstance->mTipMsgDllData;
		auto data = dataMap.find(type);
		if (data == dataMap.end())
		{
			throw invalid_argument(format("I10n Tip Config not exist this type {}", PB_TipCode_Name(type)));
		}

		return (data->second->*(PInstance->pTipMsgFunc))().c_str();
	}

}