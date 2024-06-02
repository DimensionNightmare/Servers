module;
#include <fstream>
#include <format>

#include "l10n/l10n.pb.h"
export module I10nText;

import Config.Server;

using namespace std;
using namespace l10n;

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
	const char* InitConfigData();
public:
	unique_ptr<L10nErr> pErrMsgData;
	unique_ptr<L10nTip> pTipMsgData;

	ErrTextFunc pErrMsgFunc = nullptr;
	TipTextFunc pTipMsgFunc = nullptr;

	LangType eType = LangType::zh_CN;
};

DNl10n::DNl10n()
{}

DNl10n* PInstance = nullptr;

export void SetDNl10nInstance(DNl10n* point)
{
	PInstance = point;
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
	if (!PInstance || !ErrCode_IsValid(type))
	{
		return nullptr;
	}

	auto& dataMap = PInstance->pErrMsgData->data_map();
	auto data = dataMap.find(type);
	if (data == dataMap.end())
	{
		throw invalid_argument(format("I10n Err Config not exist this type {}", ErrCode_Name(type)).c_str());
	}

	return (data->second.*(PInstance->pErrMsgFunc))().c_str();
}

export const char* GetTipText(int type)
{
	if (!PInstance || !TipCode_IsValid(type))
	{
		return nullptr;
	}

	auto& dataMap = PInstance->pTipMsgData->data_map();
	auto data = dataMap.find(type);
	if (data == dataMap.end())
	{
		throw invalid_argument(format("I10n Tip Config not exist this type {}", TipCode_Name(type)).c_str());
	}

	return (data->second.*(PInstance->pTipMsgFunc))().c_str();
}