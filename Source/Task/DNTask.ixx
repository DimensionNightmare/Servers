module;
#include <coroutine>
#include <bitset>
export module DNTask;

using namespace std;

export enum class DNTaskFlag : uint16_t
{
	Timeout = 0,
	PaserError,
	Combine,
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
	}

	void await_resume() noexcept
	{
	}
	// Awaitable

	DNTask(HandleType handle)
	{
		tHandle = handle;
		pCallPause = nullptr;
		iTimerId = 0;
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
	{
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
