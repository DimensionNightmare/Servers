module;
#include <fstream>
#include <format>

#include "l10n.pb.h"
export module I10nText;

import Config.Server;

using namespace std;
using namespace l10n;

enum class LangType
{
	zh_CN,
	en_US,
};

export class DNl10n
{
	typedef const string& (ErrText::*ErrTextFunc)() const;
	typedef const string& (TipText::*TipTextFunc)() const;
public:
	DNl10n();
	bool InitConfigData();
public:
	L10nErr* pErrMsgData;
	L10nTip* pTipMsgData;

	ErrTextFunc pErrMsgFunc;
	TipTextFunc pTipMsgFunc;

	LangType eType;
};

DNl10n::DNl10n()
{
	pErrMsgData = nullptr;
	pTipMsgData = nullptr;
	eType = LangType::zh_CN;
}

DNl10n* PInstance = nullptr;

export void SetDNl10nInstance(DNl10n* point)
{
	PInstance = point;
}

bool DNl10n::InitConfigData()
{
	string* value = GetLuanchConfigParam("l10nErrPath");
	if (!value)
	{
		printf("Launch Param l10nErrPath Error !\n");
		return false;
	}

	if(pErrMsgData)
	{
		pErrMsgData->Clear();
	}
	else
	{
		pErrMsgData = new L10nErr();
	}

	{
		ifstream input(*value, ios::in | ios::binary);
		if(!input || !pErrMsgData->ParseFromIstream(&input))
		{
			printf("load I10n Err Config Error !\n");
			return false;
		}
	}
	
	value = GetLuanchConfigParam("l10nTipPath");
	if (!value)
	{
		printf("Launch Param l10nTipPath Error Error !\n");
		return false;
	}

	if(pTipMsgData)
	{
		pTipMsgData->Clear();
	}
	else
	{
		pTipMsgData = new L10nTip();
	}

	{
		ifstream input(*value, ios::in | ios::binary);
		if(!input || !pTipMsgData->ParseFromIstream(&input))
		{
			printf("load I10n Tip Config Error !\n");
			return false;
		}
	}
	
	value = GetLuanchConfigParam("l10nLang");
	if (value)
	{
		eType = (LangType)stoi(*value);
	}

	switch (eType)
	{
	case LangType::zh_CN :
	{
		pErrMsgFunc = &ErrText::zh;
		pTipMsgFunc = &TipText::zh;
		break;
	}
	case LangType::en_US :
	{
		pErrMsgFunc = &ErrText::en;
		pTipMsgFunc = &TipText::en;
		break;
	}
	default:
		printf("load I10n Lang Type Error !\n");
		return false;
	}

	SetDNl10nInstance(this);

	return true;
}

export const char* GetErrText(ErrCode type)
{
	if(!PInstance || type < ErrCode_MIN || type > ErrCode_MAX)
	{
		return nullptr;
	}
	
	auto& dataMap = PInstance->pErrMsgData->data_map();
	auto data = dataMap.find((int)type);
	if(data == dataMap.end())
	{
		throw new exception(format("I10n Err Config not exist this type {}", ErrCode_Name(type)).c_str()); 
	}
	
	return (data->second.*(PInstance->pErrMsgFunc))().c_str();
}

export const char* GetTipText(TipCode type)
{
	if(!PInstance || type < TipCode_MIN || type > TipCode_MAX)
	{
		return nullptr;
	}
	
	auto& dataMap = PInstance->pTipMsgData->data_map();
	auto data = dataMap.find((int)type);
	if(data == dataMap.end())
	{
		throw new exception(format("I10n Tip Config not exist this type {}", TipCode_Name(type)).c_str()); 
	}
	
	return (data->second.*(PInstance->pTipMsgFunc))().c_str();
}