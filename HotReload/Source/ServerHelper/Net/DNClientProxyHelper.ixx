module;
#include "hv/TcpClient.h"

#include <functional>
export module DNClientProxyHelper;

import DNClientProxy;
import AfxCommon;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr(), __FUNCTION__, ##__VA_ARGS__);

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

	void UpdateClientState(Channel::Status state);
	void StartRegist();
	void ServerDisconnect();
};

module:private;

void DNClientProxyHelper::SetRegistEvent(function<void()> event)
{
	pRegistEvent = event;
}

void DNClientProxyHelper::UpdateClientState(Channel::Status state)
{
	if(state == eState)
	{
		return;
	}

	eState = state;

	switch (eState)
	{
	case Channel::Status::CONNECTED :
		StartRegist();
		break;

	case Channel::Status::CLOSED :
	case Channel::Status::DISCONNECTED :
		ServerDisconnect();
	default:
		break;
	}

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
				DNPrintErr("Not RegistEvent to Call!! \n");
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
}
