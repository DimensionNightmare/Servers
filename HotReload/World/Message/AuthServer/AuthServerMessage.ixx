module;
export module AuthServerMessage;

import :AuthCommon;
import ApiManager;
import MessageRegister;

export class AuthServerMessageHandle : public MessageRegister
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

	std::function<void(const DNServer::Ptr& server)> GetClientRegistFunc()
	{
		return &AuthServerMessage::Evt_ReqRegistSrv;
	}
	
};
