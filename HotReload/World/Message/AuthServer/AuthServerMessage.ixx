module;
export module AuthServerMessage;

import :AuthCommon;
import ApiManager;
import MessageRegister;

export class AuthServerMessageHandle : public MessageRegister
{

public:

	void RegApiHandle(DNServer::CVPtr dnServer) override
	{
		ApiInit(dnServer);
	}

	void RegMsgHandle()
	{

	}

	std::function<void(DNServer::CVPtr)> GetClientRegistFunc()
	{
		return &AuthServerMessage::Evt_ReqRegistSrv;
	}
	
};
