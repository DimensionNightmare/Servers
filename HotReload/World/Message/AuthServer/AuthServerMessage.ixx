module;
export module AuthServerMessage;

import :AuthCommon;
import ApiManager;
import MessageRegister;

export class AuthServerMessageHandle : public MessageRegister
{

public:

	void RegApiHandle(Server::CVPtr dnServer) override
	{
		ApiInit(dnServer);
	}

	void RegMsgHandle()
	{

	}

	std::function<void(Server::CVPtr)> GetClientRegistFunc()
	{
		return &AuthServerMessage::Evt_ReqRegistSrv;
	}
	
};
