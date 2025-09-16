export module Timer;

import ThirdParty.Libhv;
import ECSW;

export class Timer : public System
{
protected:
	friend class World;
	friend class UniversalMemoryPool;
	/// @brief
	Timer(World::WPtr world):System(world)
	{
		emSystemType = EMSystemType::Timer;

		pLoop = MemPool->Allocate<hv::EventLoopThread>();

	}
public:
	using Ptr = std::shared_ptr<Timer>;
	using CVPtr = const Ptr&;
	using WPtr = std::weak_ptr<Timer>;

	/// @brief
	virtual ~Timer()
	{
		
	}

	virtual bool Awake() override
	{
		GetWorld()->AddEvent(EMEventType::ServerStart, GetSelfW<Timer>(), &Timer::Start);
		return true;
	}

	virtual void Dispose() override
	{
		pLoop = nullptr;
		System::Dispose();
	}

	void Start()
	{
		pLoop->start();
	}

	void Stop()
	{
		pLoop->stop(true);
	}

	void KillTimer(size_t timerId)
	{
		pLoop->loop()->killTimer(timerId);
	}

	uint64_t SetTimeout(uint64_t milliseconds, const std::function<void(uint64_t)>& cb)
	{
		return pLoop->loop()->setTimeout(milliseconds, cb);
	}

	uint64_t SetInterval(uint64_t milliseconds, const std::function<void(uint64_t)>& cb)
	{
		return pLoop->loop()->setInterval(milliseconds, cb);
	}

protected:

	std::shared_ptr<hv::EventLoopThread> pLoop;

};
