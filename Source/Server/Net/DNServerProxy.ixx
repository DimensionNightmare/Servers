module;
#include "StdMacro.h"
export module DNServerProxy;

import DNTask;
import MessagePack;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

template<class TSocketChannel = SocketChannel>
class TcpServerEventLoopTmpl
{
public:
	typedef std::shared_ptr<TSocketChannel> TSocketChannelPtr;

	TcpServerEventLoopTmpl(EventLoopPtr loop = nullptr)
	{
		acceptor_loop = loop ? loop : std::make_shared<EventLoop>();
		port = 0;
		listenfd = -1;
		tls = false;
		tls_setting = nullptr;
		unpack_setting = nullptr;
		max_connections = 0xFFFFFFFF;
		load_balance = LB_RoundRobin;
	}

	virtual ~TcpServerEventLoopTmpl()
	{
		HVExport::HV_FREE(tls_setting);
		HVExport::HV_FREE(unpack_setting);
	}

	EventLoopPtr loop(int idx = -1)
	{
		EventLoopPtr worker_loop = worker_threads.loop(idx);
		if (worker_loop == nullptr)
		{
			worker_loop = acceptor_loop;
		}
		return worker_loop;
	}

	//@retval >=0 listenfd, <0 error
	int createsocket(int port, const char* host = "0.0.0.0")
	{
		listenfd = HVExport::Listen(port, host);
		if (listenfd < 0) return listenfd;
		this->host = host;
		this->port = port;
		return listenfd;
	}
	// closesocket thread-safe
	void closesocket()
	{
		if (listenfd >= 0)
		{
			hloop_t* loop = acceptor_loop->loop();
			if (loop)
			{
				hio_t* listenio = HVExport::hio_get(loop, listenfd);
				ASSERT(listenio != nullptr);
				HVExport::hio_close_async(listenio);
			}
			listenfd = -1;
		}
	}

	void setMaxConnectionNum(uint32_t num)
	{
		max_connections = num;
	}

	void setLoadBalance(load_balance_e lb)
	{
		load_balance = lb;
	}

	// NOTE: totalThreadNum = 1 acceptor_thread + N worker_threads (N can be 0)
	void setThreadNum(int num)
	{
		worker_threads.setThreadNum(num);
	}

	int startAccept()
	{
		if (listenfd < 0)
		{
			listenfd = createsocket(port, host.c_str());
			if (listenfd < 0)
			{
				return listenfd;
			}
		}
		hloop_t* loop = acceptor_loop->loop();
		if (loop == nullptr) return -2;
		hio_t* listenio = HVExport::haccept(loop, listenfd, onAccept);
		ASSERT(listenio != nullptr);
		((hevent_t*)(listenio))->userdata = (void*)this;
		if (tls)
		{
			HVExport::hio_enable_ssl(listenio);
			if (tls_setting)
			{
				int ret = HVExport::hio_new_ssl_ctx(listenio, tls_setting);
				if (ret != 0)
				{
					closesocket();
					return ret;
				}
			}
		}
		return 0;
	}

	int stopAccept()
	{
		if (listenfd < 0) return -1;
		hloop_t* loop = acceptor_loop->loop();
		if (loop == nullptr) return -2;
		hio_t* listenio = hio_get(loop, listenfd);
		assert(listenio != nullptr);
		return hio_del(listenio, 1);
	}

	// start thread-safe
	void start(bool wait_threads_started = true)
	{
		if (worker_threads.threadNum() > 0)
		{
			worker_threads.start(wait_threads_started);
		}
		acceptor_loop->runInLoop(std::bind(&TcpServerEventLoopTmpl::startAccept, this));
	}
	// stop thread-safe
	void stop(bool wait_threads_stopped = true)
	{
		closesocket();
		if (worker_threads.threadNum() > 0)
		{
			worker_threads.stop(wait_threads_stopped);
		}
	}

	int withTLS(hssl_ctx_opt_t* opt = nullptr)
	{
		tls = true;
		if (opt)
		{
			if (tls_setting == nullptr)
			{
				HVExport::HV_ALLOC_SIZEOF(tls_setting, sizeof(hssl_ctx_opt_t));
			}
			opt->endpoint = 0;
			*tls_setting = *opt;
		}
		return 0;
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
			unpack_setting = (unpack_setting_t*)HVExport::HV_ALLOC_SIZEOF(sizeof(unpack_setting_t));;
		}
		*unpack_setting = *setting;
	}

	// channel
	const TSocketChannelPtr& addChannel(hio_t* io)
	{
		uint32_t id = HVExport::hio_id(io);
		auto channel = std::make_shared<TSocketChannel>(io);
		std::lock_guard<std::mutex> locker(mutex_);
		channels[id] = channel;
		return channels[id];
	}

	TSocketChannelPtr getChannelById(uint32_t id)
	{
		std::lock_guard<std::mutex> locker(mutex_);
		auto iter = channels.find(id);
		return iter != channels.end() ? iter->second : nullptr;
	}

	void removeChannel(const TSocketChannelPtr& channel)
	{
		uint32_t id = channel->id();
		std::lock_guard<std::mutex> locker(mutex_);
		channels.erase(id);
	}

	size_t connectionNum()
	{
		std::lock_guard<std::mutex> locker(mutex_);
		return channels.size();
	}

	int foreachChannel(std::function<void(const TSocketChannelPtr& channel)> fn)
	{
		std::lock_guard<std::mutex> locker(mutex_);
		for (auto& pair : channels)
		{
			fn(pair.second);
		}
		return channels.size();
	}

	// broadcast thread-safe
	int broadcast(const void* data, int size)
	{
		return foreachChannel([data, size](const TSocketChannelPtr& channel)
		{
			channel->write(data, size);
		});
	}

	int broadcast(const std::string& str)
	{
		return broadcast(str.data(), str.size());
	}

private:
	static void newConnEvent(hio_t* connio)
	{
		TcpServerEventLoopTmpl* server = static_cast<TcpServerEventLoopTmpl*>(((hevent_t*)(connio))->userdata);
		if (server->connectionNum() >= server->max_connections)
		{
			HVExport::hio_close(connio);
			return;
		}

		// NOTE: attach to worker loop
		EventLoop* worker_loop = HVExport::tlsEventLoop();
		ASSERT(worker_loop != nullptr);
		HVExport::hio_attach(worker_loop->loop(), connio);

		const TSocketChannelPtr& channel = server->addChannel(connio);
		channel->status = SocketChannel::CONNECTED;

		channel->onread = [server, &channel](Buffer* buf)
		{
			if (server->onMessage)
			{
				server->onMessage(channel, buf);
			}
		};
		channel->onwrite = [server, &channel](Buffer* buf)
		{
			if (server->onWriteComplete)
			{
				server->onWriteComplete(channel, buf);
			}
		};
		channel->onclose = [server, &channel]()
		{
			EventLoop* worker_loop = HVExport::tlsEventLoop();
			ASSERT(worker_loop != nullptr);
			--worker_loop->connectionNum;

			channel->status = SocketChannel::CLOSED;
			if (server->onConnection)
			{
				server->onConnection(channel);
			}
			server->removeChannel(channel);
			// NOTE: After removeChannel, channel may be destroyed,
			// so in this lambda function, no code should be added below.
		};

		if (server->unpack_setting)
		{
			channel->setUnpack(server->unpack_setting);
		}
		channel->startRead();
		if (server->onConnection)
		{
			server->onConnection(channel);
		}
	}

	static void onAccept(hio_t* connio)
	{
		TcpServerEventLoopTmpl* server =  static_cast<TcpServerEventLoopTmpl*>(((hevent_t*)(connio))->userdata);
		// NOTE: detach from acceptor loop
		HVExport::hio_detach(connio);
		EventLoopPtr worker_loop = server->worker_threads.nextLoop(server->load_balance);
		if (worker_loop == nullptr)
		{
			worker_loop = server->acceptor_loop;
		}
		++worker_loop->connectionNum;
		worker_loop->runInLoop(std::bind(&TcpServerEventLoopTmpl::newConnEvent, connio));
	}

public:
	std::string             host;
	int                     port;
	int                     listenfd;
	bool                    tls;
	hssl_ctx_opt_t* tls_setting;
	unpack_setting_t* unpack_setting;
	// Callback
	std::function<void(const TSocketChannelPtr&)>           onConnection;
	std::function<void(const TSocketChannelPtr&, Buffer*)>  onMessage;
	// NOTE: Use Channel::isWriteComplete in onWriteComplete callback to determine whether all data has been written.
	std::function<void(const TSocketChannelPtr&, Buffer*)>  onWriteComplete;

	uint32_t                max_connections;
	load_balance_e          load_balance;

private:
	// id => TSocketChannelPtr
	std::map<uint32_t, TSocketChannelPtr>   channels; // GUAREDE_BY(mutex_)
	std::mutex                              mutex_;

	EventLoopPtr            acceptor_loop;
	EventLoopThreadPool     worker_threads;
};

template<class TSocketChannel = SocketChannel>
class TcpServerTmpl : public EventLoopThread, public TcpServerEventLoopTmpl<TSocketChannel>
{
public:
	TcpServerTmpl(EventLoopPtr loop = nullptr)
		: EventLoopThread(loop)
		, TcpServerEventLoopTmpl<TSocketChannel>(EventLoopThread::loop())
		, is_loop_owner(loop == nullptr)
	{
	}
	virtual ~TcpServerTmpl()
	{
		stop(true);
	}

	EventLoopPtr loop(int idx = -1)
	{
		return TcpServerEventLoopTmpl<TSocketChannel>::loop(idx);
	}

	// start thread-safe
	void start(bool wait_threads_started = true)
	{
		TcpServerEventLoopTmpl<TSocketChannel>::start(wait_threads_started);
		if (!isRunning())
		{
			EventLoopThread::start(wait_threads_started);
		}
	}

	// stop thread-safe
	void stop(bool wait_threads_stopped = true)
	{
		if (is_loop_owner)
		{
			EventLoopThread::stop(wait_threads_stopped);
		}
		TcpServerEventLoopTmpl<TSocketChannel>::stop(wait_threads_stopped);
	}

private:
	bool is_loop_owner;
};

export class DNServerProxy : public TcpServerTmpl<SocketChannel>
{

public:

	DNServerProxy()
	{
		pLoop = make_shared<EventLoopThread>();
	}

	~DNServerProxy()
	{
		pLoop = nullptr;
		mMsgList.clear();
		mMapTimer.clear();
	}

	void Init()
	{
		// if not set port mean need get port by self 
		if (!port && listenfd > 0)
		{
			sockaddr_in addr;
			int addrLen = sizeof(addr);
			if (HVExport::getsockname(listenfd, reinterpret_cast<struct sockaddr*>(&addr), &addrLen) < 0)
			{
				DNPrint(ErrCode::ErrCode_GetSocketName, LoggerLevel::Error, nullptr);
				return;
			}

			port = HVExport::ntohs(addr.sin_port);
		}

		unpack_setting_t setting;
		setting.mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
		setting.length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
		setting.body_offset = MessagePacket::PackLenth;
		setting.length_field_bytes = 1;
		setting.length_field_offset = 0;
		setUnpack(&setting);
		setThreadNum(4);
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

	void InitConnectedChannel(const SocketChannelPtr& channel)
	{
		// if not regist
		CheckChannelByTimer(channel);
		// if not recive data

		// channel->setReadTimeout(15000);
	}

	void MessageTimeoutTimer(uint64_t timerID)
	{
		uint32_t id = -1;
		{
			if (!mMapTimer.contains(timerID))
			{
				return;
			}

			unique_lock<shared_mutex> ulock(oTimerMutex);
			id = mMapTimer[timerID];
			mMapTimer.erase(timerID);
		}

		{
			if (mMsgList.contains(id))
			{
				unique_lock<shared_mutex> ulock(oMsgMutex);
				DNTask<Message*>* task = mMsgList[id];
				mMsgList.erase(id);
				task->SetFlag(DNTaskFlag::Timeout);
				task->CallResume();
			}
		}

	}

	void ChannelTimeoutTimer(uint64_t timerID)
	{
		uint32_t id = -1;
		{
			if (!mMapTimer.contains(timerID))
			{
				return;
			}

			unique_lock<shared_mutex> ulock(oTimerMutex);
			id = mMapTimer[timerID];
			mMapTimer.erase(timerID);
		}

		{
			if (TSocketChannelPtr channel = getChannelById(id))
			{
				if (!channel->context())
				{
					channel->close();
					DNPrint(0, LoggerLevel::Debug, "ChannelTimeoutTimer server destory entity\n");
				}
			}
		}

	}

	const EventLoopPtr& Timer() { return pLoop->loop(); }

	void AddTimerRecord(size_t timerId, uint32_t id)
	{
		unique_lock<shared_mutex> ulock(oTimerMutex);
		mMapTimer.emplace(timerId, id);
	}

	void CheckChannelByTimer(SocketChannelPtr channel)
	{
		size_t timerId = Timer()->setTimeout(5000, std::bind(&DNServerProxy::ChannelTimeoutTimer, this, placeholders::_1));
		AddTimerRecord(timerId, channel->id());
	}
	uint64_t CheckMessageTimeoutTimer(uint32_t breakTime, uint32_t msgId)
	{
		uint64_t timerId = Timer()->setTimeout(breakTime, std::bind(&DNServerProxy::MessageTimeoutTimer, this, placeholders::_1));
		unique_lock<shared_mutex> ulock(oTimerMutex);
		mMapTimer[timerId] = msgId;
		return timerId;
	}
public:
	// cant init in tcpclient this class
	shared_ptr<EventLoopThread> pLoop;

protected:
	// only oddnumber
	atomic<uint32_t> iMsgId;
	// unordered_
	unordered_map<uint32_t, DNTask<Message*>* > mMsgList;
	//
	unordered_map<uint64_t, uint32_t > mMapTimer;

	shared_mutex oMsgMutex;

	shared_mutex oTimerMutex;
};
