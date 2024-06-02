module;
#include <coroutine>
#include <bitset>
#include <chrono>
#include <format>
#include <iostream>
export module DNTask;

using namespace std;
using namespace std::chrono;

export enum class DNTaskFlag : uint16_t
{
	Timeout = 0,
	PaserError,
	Combine,
	TimeCost,
	Max,
};

constexpr size_t DNTaskFlagSize()
{
	return static_cast<size_t>(DNTaskFlag::Max);
}

export template <typename T>
struct DNTask
{
	struct promise_type;
	using HandleType = coroutine_handle<promise_type>;
	struct promise_type
	{
		promise_type()
		{
			oResult = nullptr;
		}

		DNTask get_return_object()
		{
			return DNTask{ HandleType::from_promise(*this) };
		}

		void return_value(const T& value)
		{
			oResult = &value;
		}

		suspend_always initial_suspend() { return {}; }

		suspend_always final_suspend() noexcept { return {}; }

		void unhandled_exception() {}

		const T& GetResult() const { return *oResult; }

		const T* oResult;
	};

	// Awaitable
	bool await_ready() const noexcept
	{
		return tHandle.done();
	}

	void await_suspend(coroutine_handle<> caller)
	{
		pCallPause = caller;
		if (HasFlag(DNTaskFlag::TimeCost))
		{
			oTimePoint = steady_clock::now();
		}
	}

	void await_resume() noexcept
	{}
	// Awaitable

	DNTask(HandleType handle)
	{
		tHandle = handle;
		pCallPause = nullptr;
		iTimerId = 0;
		// SetFlag(DNTaskFlag::TimeCost);
	}

	~DNTask()
	{
		Destroy();
	}

	void Resume()
	{
		if (!tHandle || tHandle.done())
		{
			return;
		}

		tHandle.resume();
	}

	void CallResume()
	{
		if (!pCallPause)
		{
			return;
		}

		pCallPause.resume();
	}

	const T& GetResult()
	{
		return tHandle.promise().GetResult();
	}

	void Destroy()
	{
		if (HasFlag(DNTaskFlag::TimeCost))
		{
			steady_clock::time_point now = steady_clock::now();
			cout << format("tasktimeid:{}, cost:{}ms", iTimerId, duration_cast<microseconds>(now - oTimePoint).count() / 1000.0) << endl;
		}

		if (tHandle)
		{
			tHandle.destroy();
			tHandle = nullptr;
		}
	}
public:
	bool HasFlag(DNTaskFlag flag) { return oFlags.test(int(flag)); }
	void SetFlag(DNTaskFlag flag) { oFlags.set(int(flag)); }
	void ClearFlag(DNTaskFlag flag) { oFlags.reset(int(flag)); }

	size_t& TimerId() { return iTimerId; }

private:
	HandleType tHandle;
	coroutine_handle<> pCallPause;
	bitset<DNTaskFlagSize()> oFlags;
	size_t iTimerId;

	steady_clock::time_point oTimePoint;
};

export struct DNTaskVoid
{
	struct promise_type;
	using HandleType = coroutine_handle<promise_type>;
	struct promise_type
	{
		promise_type() {}

		void return_void() {}

		DNTaskVoid get_return_object()
		{
			return DNTaskVoid{ HandleType::from_promise(*this) };
		}

		suspend_never initial_suspend() { return {}; }

		suspend_never final_suspend() noexcept { return {}; }

		void unhandled_exception() {}
	};

	// Awaitable Start
	bool await_ready() const noexcept
	{
		return tHandle.done();
	}

	void await_suspend(coroutine_handle<> caller)
	{}

	void await_resume() noexcept
	{}
	// Awaitable End

	DNTaskVoid(HandleType handle)
	{
		tHandle = handle;
	}

	void Resume()
	{
		if (!tHandle || tHandle.done())
		{
			return;
		}

		tHandle.resume();
	}

	HandleType tHandle;
};
