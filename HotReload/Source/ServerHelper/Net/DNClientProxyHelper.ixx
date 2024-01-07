module;
#include "hv/TcpClient.h"

#include <functional>
export module DNClientProxyHelper;

import DNClientProxy;

using namespace std;
using namespace hv;

export class DNClientProxyHelper : public DNClientProxy
{
private:
	DNClientProxyHelper();
public:

	auto GetMsgId() { return ++iMsgId; }

	auto& GetMsgMap(){ return mMsgList; }

	// regist to controlserver
	bool IsRegisted(){return bIsRegisted;}
	void SetRegisted(bool isRegisted){bIsRegisted = isRegisted;}
	void SetRegistEvent(function<void()> event);
	void StartRegist();

	void ServerDisconnect();
};

module:private;

void DNClientProxyHelper::SetRegistEvent(function<void()> event)
{
	pRegistEvent = event;
}

void DNClientProxyHelper::StartRegist()
{
	// setInterval can!t runtime modify
	loop()->setInterval(1000, [this](TimerID timerID)
	{
		if (channel->isConnected() && !IsRegisted()) 
		{
			if(pRegistEvent)
			{
				pRegistEvent();
			}
			else
			{
				printf("%s->Not RegistEvent to Call!! \n", __FUNCTION__);
			}
		} 
		else 
		{
			loop()->killTimer(timerID);
		}
	});
}

void DNClientProxyHelper::ServerDisconnect()
{
	SetRegisted(false);

	for(auto& [k,v] : mMsgList)
	{
		v->Destroy();
	}
		
	mMsgList.clear();

	startReconnect();
}
