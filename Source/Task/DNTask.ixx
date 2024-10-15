module;
#include "StdMacro.h"
export module DNTask;

import ThirdParty.PbGen;

using namespace std::chrono;

export enum class DNTaskFlag : uint16_t
{
	Timeout = 0,
	PaserError,
	TimeCost,
	Max,
};

constexpr uint16_t DNTaskFlagSize() { return static_cast<uint16_t>(DNTaskFlag::Max); }

export template <typename T>
struct DNTask
{
	struct promise_type;
	using HandleType = coroutine_handle<promise_type>;
	struct promise_type
	{
		promise_type()
		{
		}

		DNTask get_return_object()
		{
			return DNTask{ HandleType::from_promise(*this) };
		}

		void return_value(const T& value)
		{
			oResult = &value;
			bReturned = true;
		}

		suspend_always initial_suspend() { return {}; }

		suspend_always final_suspend() noexcept
		{
			// DNTask don't Call by self, need Message handle Tick;
			// ReleaseAwaitHandle();
			return {};
		}

		void unhandled_exception() {}

		const T& GetResult() const { return *oResult; }

		void ReleaseAwaitHandle()
		{
			if (oAwaitHandle) { oAwaitHandle.resume(); oAwaitHandle = nullptr; }
		}

		const T* oResult = nullptr;

		coroutine_handle<> oAwaitHandle = nullptr;

		bool bReturned = false;
	};

	// Awaitable
	bool await_ready() const noexcept
	{
		return tHandle.promise().bReturned;
	}

	void await_suspend(coroutine_handle<> caller)
	{
		tHandle.promise().oAwaitHandle = caller;

		if (HasFlag(DNTaskFlag::TimeCost))
		{
			oTimePoint = steady_clock::now();
		}
	}

	void await_resume() noexcept
	{
	}
	// Awaitable

	DNTask(HandleType handle)
	{
		tHandle = handle;
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
		tHandle.promise().ReleaseAwaitHandle();
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
	bool HasFlag(DNTaskFlag flag) { return oFlags.test(uint16_t(flag)); }
	void SetFlag(DNTaskFlag flag) { oFlags.set(uint16_t(flag)); }
	void ClearFlag(DNTaskFlag flag) { oFlags.reset(uint16_t(flag)); }

	size_t& TimerId() { return iTimerId; }

private:
	HandleType tHandle;
	bitset<DNTaskFlagSize()> oFlags;
	size_t iTimerId = 0;

	steady_clock::time_point oTimePoint;
};

export struct DNTaskVoid
{
	struct promise_type;
	using HandleType = coroutine_handle<promise_type>;
	struct promise_type
	{
		promise_type() {}

		void return_void() { bReturned = true; }

		DNTaskVoid get_return_object()
		{
			return DNTaskVoid{ HandleType::from_promise(*this) };
		}

		suspend_never initial_suspend() { return {}; }

		suspend_never final_suspend() noexcept
		{
			ReleaseAwaitHandle();
			return {};
		}

		void unhandled_exception() {}

		void ReleaseAwaitHandle()
		{
			if (oAwaitHandle) { oAwaitHandle.resume(); oAwaitHandle = nullptr; }
		}

		coroutine_handle<> oAwaitHandle = nullptr;

		bool bReturned = false;
	};

	// Awaitable Start
	bool await_ready() const noexcept
	{
		return tHandle.promise().bReturned;
	}

	void await_suspend(coroutine_handle<> caller)
	{
		tHandle.promise().oAwaitHandle = caller;
	}

	void await_resume() noexcept
	{
	}
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
