module;
export module AuthMessage;

export import :AuthCommon;
import ApiManager;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import Logger;
import std.compat;
import StrUtils;
import MessageRegister;

export class AuthMessageHandle : public MessageRegister
{

public:

	static void RegApiHandle(DNServer::WPtr server, hv::HttpService* service)
	{
		service->Static("/", "./");

		ApiInit(server, service);
	}

	void RegMsgHandle()
	{

	}
};
