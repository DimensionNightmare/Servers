module;
#include "hv/TcpClient.h"

#include <functional>
#include <shared_mutex>
export module DNClientProxyHelper;

import DNClientProxy;
import AfxCommon;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);

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

	bool AddMsg(unsigned int msgId, DNTask<void*>* msg);
	DNTask<void*>* GetMsg(unsigned int msgId);
	void DelMsg(unsigned int msgId);
};

#pragma region // ClientReconnectFunc

static std::function<void(const char*, int)> PClientReconnectFunc = nullptr;

export void SetClientReconnectFunc( std::function<void(const char*, int)> func)
{
	PClientReconnectFunc = func;
}

export std::function<void(const char*, int)> GetClientReconnectFunc()
{
	return PClientReconnectFunc;
}

#pragma endregion

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
	loop()->setInterval(1000, [this](uint64_t timerID)
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

bool DNClientProxyHelper::AddMsg(unsigned int msgId, DNTask<void *> *msg)
{
	unique_lock<shared_mutex> ulock(oMsgMutex);
	mMsgList.emplace(msgId, msg);
	return true;
}

DNTask<void *> *DNClientProxyHelper::GetMsg(unsigned int msgId)
{;
	shared_lock<shared_mutex> lock(oMsgMutex);
	if(mMsgList.contains(msgId))
	{
		return mMsgList[msgId];
	}
	return nullptr;
}

void DNClientProxyHelper::DelMsg(unsigned int msgId)
{
	unique_lock<shared_mutex> ulock(oMsgMutex);
	mMsgList.erase(msgId);
}
