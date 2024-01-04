module;

#include "hv/TcpServer.h"
#include "hv/TcpClient.h"
export module DNServerHelper;

using namespace std;
import DNServer;


export class DNServerProxyHelper : public DNServerProxy
{
private:
	DNServerProxyHelper(){};

public:
	// DNServerProxyHelper* GetSelf(){ return nullptr;}
};

export class DNClientProxyHelper : public DNClientProxy
{
private:
	DNClientProxyHelper();
public:
	// DNClientProxyHelper* GetSelf(){ return nullptr;}

	auto GetMsgId() { return ++iMsgId; }

	auto& GetMsgMap(){ return mMsgList; }

	bool IsRegisted(){return bIsRegisted;}
	void SetRegisted(bool isRegisted){bIsRegisted = isRegisted;}
};

module:private;


