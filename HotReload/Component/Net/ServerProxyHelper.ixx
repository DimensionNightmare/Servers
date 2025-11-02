export module ServerProxyHelper;

import ServerProxy;
import FuncUtils;
import FuncHelper;
import MessagePack;
import ThirdParty.Protobuf;

export class ServerProxyHelper : public Helper<ServerProxyHelper, ServerProxy>
{
	
private:

	ServerProxyHelper() = delete;
	~ServerProxyHelper() = default;

public:

	uint32_t GetMsgId() { return ++iMsgId; }

	Task<bool> AddMsg(EMMsgDeal dealType, Message* request, Message* response, SocketChannel::Ptr channel, uint32_t breakTime = 10000)
	{
		int msgId = 0;

		switch(dealType)
		{
			case EMMsgDeal::Req:
			case EMMsgDeal::Redir:
			{
				msgId = GetMsgId();
				break;
			}
			case EMMsgDeal::Ret:
			{
				break;
			}

		}

		auto task = MakeMsgTask();

		auto send = [&](){
			MessagePackAndSend(msgId, dealType, request, channel);
		};
		
		if(msgId)
		{
			task.SetMessage(response);
			task.SetCallback(send);

			{
				std::unique_lock ulock(oMsgMutex);
				mMsgList.emplace(msgId, &task);
			}

			// timeout
			if (breakTime > 0)
			{
				task.SetTimerId(CheckMessageTimeoutTimer(breakTime, msgId));
			}

			co_await task;

			if (task.HasFlag(EMTaskFlag::Timeout))
			{
				// response->set_errorcode(EL10nCode_CRdbReqTimeout);
				co_return false;
			}

		}
		else
		{
			send();
		}

		co_return true;
	}

	MsgTask* GetMsg(uint32_t msgId)
	{
		std::shared_lock lock(oMsgMutex);
		if (mMsgList.contains(msgId))
		{
			return mMsgList[msgId];
		}
		return nullptr;
	}

	void DelMsg(uint32_t msgId)
	{
		std::unique_lock ulock(oMsgMutex);
		if (mMsgList.contains(msgId))
		{
			if (MsgTask* task = mMsgList[msgId])
			{
				if (size_t timerId = task->GetTimerId())
				{
					GetTimer()->KillTimer(timerId);
					mMapTimer.erase(timerId);
				}
			}
		}
		mMsgList.erase(msgId);
	}

	void ClearMsgMap()
	{
		std::unique_lock ulock(oMsgMutex);
		for (auto& [k, v] : mMsgList)
		{
			v->CallResume();
		}
		mMsgList.clear();
	}
};
