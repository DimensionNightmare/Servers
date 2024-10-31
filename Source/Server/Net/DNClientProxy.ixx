module;
#include "StdMacro.h"
export module DNClientProxy;

import DNTask;
import FuncHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import MessagePack;

#define NABS(n) ((n) < 0 ? (n) : -(n))

export enum class EMRegistState : uint8_t
{
	None,
	Registing,
	Registed,
};

template<class TSocketChannel = SocketChannel>
class TcpClientEventLoopTmpl
{
public:
	typedef std::shared_ptr<TSocketChannel> TSocketChannelPtr;

	TcpClientEventLoopTmpl(EventLoopPtr loop = nullptr)
	{
		loop_ = loop ? loop : std::make_shared<EventLoop>();
		remote_port = 0;
		connect_timeout = 10000;
		tls = false;
		tls_setting = nullptr;
		reconn_setting = nullptr;
		unpack_setting = nullptr;
	}

	virtual ~TcpClientEventLoopTmpl()
	{
		HVExport::HV_FREE(tls_setting);
		HVExport::HV_FREE(reconn_setting);
		HVExport::HV_FREE(unpack_setting);
	}

	const EventLoopPtr& loop()
	{
		return loop_;
	}

	// delete thread-safe
	void deleteInLoop()
	{
		loop_->runInLoop([this]()
		{
			delete this;
		});
	}

	// NOTE: By default, not bind local port. If necessary, you can call bind() after createsocket().
	// @retval >=0 connfd, <0 error
	int createsocket(int remote_port, const char* remote_host = "127.0.0.1")
	{
		memset(&remote_addr, 0, sizeof(remote_addr));
		int ret = HVExport::sockaddr_set_ipport(&remote_addr, remote_host, remote_port);
		if (ret != 0)
		{
			return NABS(ret);
		}
		this->remote_host = remote_host;
		this->remote_port = remote_port;
		return createsocket(&remote_addr.sa);
	}

	int createsocket(struct sockaddr* remote_addr)
	{
		int connfd = HVExport::socket(remote_addr->sa_family, 1, 0);
		// SOCKADDR_PRINT(remote_addr);
		if (connfd < 0)
		{
			perror("socket");
			return -2;
		}

		hio_t* io = HVExport::hio_get(loop_->loop(), connfd);
		ASSERT(io != nullptr);
		HVExport::hio_set_peeraddr(io, remote_addr, HVExport::sockaddr_len((sockaddr_u*)remote_addr));
		channel = std::make_shared<TSocketChannel>(io);
		return connfd;
	}

	int bind(int local_port, const char* local_host = "0.0.0.0")
	{
		sockaddr_u local_addr;
		memset(&local_addr, 0, sizeof(local_addr));
		int ret = HVExport::sockaddr_set_ipport(&local_addr, local_host, local_port);
		if (ret != 0)
		{
			return NABS(ret);
		}
		return bind(&local_addr.sa);
	}

	int bind(struct sockaddr* local_addr)
	{
		if (channel == nullptr || channel->isClosed())
		{
			return -1;
		}
		int ret = ::bind(channel->fd(), local_addr, HVExport::sockaddr_len(local_addr));
		if (ret != 0)
		{
			perror("bind");
		}
		return ret;
	}

	// closesocket thread-safe
	void closesocket()
	{
		if (channel && channel->status != SocketChannel::CLOSED)
		{
			loop_->runInLoop([this]()
			{
				if (channel)
				{
					setReconnect(nullptr);
					channel->close();
				}
			});
		}
	}

	int startConnect()
	{
		if (channel == nullptr || channel->isClosed())
		{
			int connfd = createsocket(&remote_addr.sa);
			if (connfd < 0)
			{
				return connfd;
			}
		}
		if (channel == nullptr || channel->status >= SocketChannel::CONNECTING)
		{
			return -1;
		}
		if (connect_timeout)
		{
			channel->setConnectTimeout(connect_timeout);
		}
		if (tls)
		{
			channel->enableSSL();
			if (tls_setting)
			{
				int ret = channel->newSslCtx(tls_setting);
				if (ret != 0)
				{
					closesocket();
					return ret;
				}
			}
			if (!HVExport::is_ipaddr(remote_host.c_str()))
			{
				channel->setHostname(remote_host);
			}
		}
		channel->onconnect = [this]()
		{
			if (unpack_setting)
			{
				channel->setUnpack(unpack_setting);
			}
			channel->startRead();
			if (onConnection)
			{
				onConnection(channel);
			}
			if (reconn_setting)
			{
				reconn_setting->cur_delay = 0;
    			reconn_setting->cur_retry_cnt = 0;
			}
		};
		channel->onread = [this](Buffer* buf)
		{
			if (onMessage)
			{
				onMessage(channel, buf);
			}
		};
		channel->onwrite = [this](Buffer* buf)
		{
			if (onWriteComplete)
			{
				onWriteComplete(channel, buf);
			}
		};
		channel->onclose = [this]()
		{
			bool reconnect = reconn_setting != nullptr;
			if (onConnection)
			{
				onConnection(channel);
			}
			if (reconnect)
			{
				startReconnect();
			}
		};
		return channel->startConnect();
	}

	int startReconnect()
	{
		if (!reconn_setting) return -1;
		if (!HVExport::reconn_setting_can_retry(reconn_setting)) return -2;
		uint32_t delay = HVExport::reconn_setting_calc_delay(reconn_setting);
		
		loop_->setTimeout(delay, [this](uint64_t timerID)
		{
			startConnect();
		});
		return 0;
	}

	// start thread-safe
	void start()
	{
		loop_->runInLoop(std::bind(&TcpClientEventLoopTmpl::startConnect, this));
	}

	bool isConnected()
	{
		if (channel == nullptr) return false;
		return channel->isConnected();
	}

	// send thread-safe
	int send(const void* data, int size)
	{
		if (!isConnected()) return -1;
		return channel->write(data, size);
	}
	int send(Buffer* buf)
	{
		return send(buf->data(), buf->size());
	}
	int send(const std::string& str)
	{
		return send(str.data(), str.size());
	}

	int withTLS(hssl_ctx_opt_t* opt = nullptr)
	{
		tls = true;
		if (opt)
		{
			if (tls_setting == nullptr)
			{
				tls_setting = (hssl_ctx_opt_t*)HVExport::HV_ALLOC_SIZEOF(sizeof(hssl_ctx_opt_t));;
			}
			opt->endpoint = 1;
			*tls_setting = *opt;
		}
		return 0;
	}

	void setConnectTimeout(int ms)
	{
		connect_timeout = ms;
	}

	void setReconnect(reconn_setting_t* setting)
	{
		if (setting == nullptr)
		{
			HVExport::HV_FREE(reconn_setting);
			reconn_setting = nullptr;
			return;
		}
		if (reconn_setting == nullptr)
		{
			reconn_setting = (reconn_setting_t*)HVExport::HV_ALLOC_SIZEOF(sizeof(reconn_setting_t));
		}
		*reconn_setting = *setting;
	}
	bool isReconnect()
	{
		return reconn_setting && reconn_setting->cur_retry_cnt > 0;
	}

	void setUnpack(unpack_setting_t* setting)
	{
		if (setting == nullptr)
		{
			HVExport::HV_FREE(unpack_setting);
			unpack_setting = nullptr;
			return;
		}
		if (unpack_setting == nullptr)
		{
			unpack_setting = (unpack_setting_t*)HVExport::HV_ALLOC_SIZEOF(sizeof(unpack_setting_t));
		}
		*unpack_setting = *setting;
	}

public:
	TSocketChannelPtr       channel;

	std::string             remote_host;
	int                     remote_port;
	sockaddr_u              remote_addr;
	int                     connect_timeout;
	bool                    tls;
	hssl_ctx_opt_t* tls_setting;
	reconn_setting_t* reconn_setting;
	unpack_setting_t* unpack_setting;

	// Callback
	std::function<void(const TSocketChannelPtr&)>           onConnection;
	std::function<void(const TSocketChannelPtr&, Buffer*)>  onMessage;
	// NOTE: Use Channel::isWriteComplete in onWriteComplete callback to determine whether all data has been written.
	std::function<void(const TSocketChannelPtr&, Buffer*)>  onWriteComplete;

private:
	EventLoopPtr            loop_;
};

template<class TSocketChannel = SocketChannel>
class TcpClientTmpl : public EventLoopThread, public TcpClientEventLoopTmpl<TSocketChannel>
{
public:
	TcpClientTmpl(EventLoopPtr loop = nullptr)
		: EventLoopThread(loop)
		, TcpClientEventLoopTmpl<TSocketChannel>(EventLoopThread::loop())
		, is_loop_owner(loop == nullptr)
	{
	}
	virtual ~TcpClientTmpl()
	{
		stop(true);
	}

	const EventLoopPtr& loop()
	{
		return EventLoopThread::loop();
	}

	// start thread-safe
	void start(bool wait_threads_started = true)
	{
		if (isRunning())
		{
			TcpClientEventLoopTmpl<TSocketChannel>::start();
		}
		else
		{
			EventLoopThread::start(wait_threads_started, [this]()
			{
				TcpClientTmpl::startConnect();
				return 0;
			});
		}
	}

	// stop thread-safe
	void stop(bool wait_threads_stopped = true)
	{
		TcpClientEventLoopTmpl<TSocketChannel>::closesocket();
		if (is_loop_owner)
		{
			EventLoopThread::stop(wait_threads_stopped);
		}
	}

private:
	bool is_loop_owner;
};

export class DNClientProxy : public TcpClientTmpl<SocketChannel>
{

public:

	DNClientProxy()
	{
		pLoop = make_shared<EventLoopThread>();
	}

	~DNClientProxy()
	{
		pLoop = nullptr;
		mMsgList.clear();
		mMapTimer.clear();
	}

	void Init()
	{
		reconn_setting_t reconn;
		reconn.min_delay = 1000;
		reconn.max_delay = 10000;
		reconn.delay_policy = 2;
		setReconnect(&reconn);

		unpack_setting_t setting;
		setting.mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
		setting.length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
		setting.body_offset = MessagePacket::PackLenth;
		setting.length_field_bytes = 1;
		setting.length_field_offset = 0;
		setUnpack(&setting);

	}

	void Start()
	{
		pLoop->start();
		start();
	}

	void End()
	{
		pLoop->stop(true);
		stop(true);
	}
public: // dll override

	void TickRegistEvent(size_t timerID)
	{
		if (eRegistState == EMRegistState::Registing)
		{
			return;
		}

		if (channel->isConnected() && eRegistState != EMRegistState::Registed)
		{
			if (pRegistEvent)
			{
				pRegistEvent();
			}
			else
			{
				DNPrint(ErrCode::ErrCode_NotCallbackEvent, EMLoggerLevel::Error, nullptr);
			}
		}
		else
		{
			Timer()->killTimer(timerID);
		}
	}

	void MessageTimeoutTimer(uint64_t timerID)
	{
		uint32_t msgId = -1;
		{
			if (!mMapTimer.contains(timerID))
			{
				return;
			}

			unique_lock<shared_mutex> ulock(oTimerMutex);
			msgId = mMapTimer[timerID];
			mMapTimer.erase(timerID);
		}

		{
			if (mMsgList.contains(msgId))
			{
				unique_lock<shared_mutex> ulock(oMsgMutex);
				DNTask<Message*>* task = mMsgList[msgId];
				mMsgList.erase(msgId);
				task->SetFlag(EMDNTaskFlag::Timeout);
				task->CallResume();
			}
		}
	}

	uint64_t CheckMessageTimeoutTimer(uint32_t breakTime, uint32_t msgId)
	{
		uint64_t timerId = Timer()->setTimeout(breakTime, std::bind(&DNClientProxy::MessageTimeoutTimer, this, placeholders::_1));
		unique_lock<shared_mutex> ulock(oTimerMutex);
		mMapTimer[timerId] = msgId;
		return timerId;
	}

	const EventLoopPtr& Timer() { return pLoop->loop(); }

	void AddTimerRecord(size_t timerId, uint32_t id)
	{
		unique_lock<shared_mutex> ulock(oTimerMutex);
		mMapTimer.emplace(timerId, id);
	}

	void TickHeartbeat()
	{
		COM_RetHeartbeat request;
		request.Clear();
		int timespan = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
		request.set_timespan(timespan);

		string binData;
		request.SerializeToString(&binData);

		MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name().c_str(), binData, GetChannel());
	}

	void InitConnectedChannel(const SocketChannelPtr& chanhel)
	{
		// chanhel->setHeartbeat(4000, std::bind(&DNClientProxy::TickHeartbeat, this));
		// channel->setWriteTimeout(12000);
		if (eRegistState == EMRegistState::None)
		{
			Timer()->setInterval(1000, std::bind(&DNClientProxy::TickRegistEvent, this, placeholders::_1));
		}
	}

	void RedirectClient(uint16_t port, string ip)
	{
		DNPrint(0, EMLoggerLevel::Debug, "reclient to %s:%u", ip.c_str(), port);

		eRegistState = EMRegistState::None;
		closesocket();
		Timer()->setTimeout(500, [=](uint64_t)
		{
			createsocket(port, ip.c_str());
			start();
		});
	}

	bool AddMsg(uint32_t msgId, DNTask<Message*>* task, uint32_t breakTime)
	{
		unique_lock<shared_mutex> ulock(oMsgMutex);
		mMsgList.emplace(msgId, task);
		// timeout
		if (breakTime > 0)
		{
			task->TimerId() = CheckMessageTimeoutTimer(breakTime, msgId);
		}
		return true;
	}

	uint8_t& RegistType() { return iRegistType; }

	uint32_t GetMsgId() { return ++iMsgId; }

	const SocketChannelPtr& GetChannel() { return channel; }

protected: // dll proxy

	shared_ptr<EventLoopThread> pLoop;

	// only oddnumber
	atomic<uint32_t> iMsgId;

	// unordered_
	unordered_map<uint32_t, DNTask<Message*>* > mMsgList;

	//
	unordered_map<uint64_t, uint32_t > mMapTimer;

	// status
	EMRegistState eRegistState = EMRegistState::None;

	uint8_t iRegistType = 0;

	function<void()> pRegistEvent;

	shared_mutex oMsgMutex;

	shared_mutex oTimerMutex;
};
