export module Timer;

import ThirdParty.Libhv;
import ECSW;

export class Timer : public System
{
protected:
	friend class UniversalMemoryPool;
	/// @brief
	Timer(World::WPtr world):System(world)
	{
		emSystemType = EMSystemType::Timer;

		pLoop = P_InstanceHolder->GetMemPool().Allocate<hv::EventLoopThread>();

	}
public:
	using Ptr = std::shared_ptr<Timer>;
	using WPtr = std::weak_ptr<Timer>;

	/// @brief
	virtual ~Timer()
	{
		
	}

	virtual bool Awake() override
	{
		GetWorld()->AddEvent<&Timer::Start>(EMEventType::ServerStart, GetSelfW<Timer>());
		GetWorld()->AddEvent<&Timer::Stop>(EMEventType::ServerStop, GetSelfW<Timer>());
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

	size_t SetTimeout(size_t milliseconds, const std::function<void(size_t)>& cb)
	{
		return pLoop->loop()->setTimeout(milliseconds, cb);
	}

	size_t SetInterval(size_t milliseconds, const std::function<void(size_t)>& cb)
	{
		return pLoop->loop()->setInterval(milliseconds, cb);
	}

protected:

	std::shared_ptr<hv::EventLoopThread> pLoop;

};
