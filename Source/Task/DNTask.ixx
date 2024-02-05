module;

#include <coroutine>
export module DNTask;

using namespace std;

export template <typename T>
struct DNTask
{
	struct promise_type;
   	using HandleType = coroutine_handle<promise_type>;
	struct promise_type
	{
		DNTask get_return_object()
		{
			return DNTask{HandleType::from_promise(*this)};
		}

		void return_value(const T& value)
		{
			oResult = &value;
		}
		
		suspend_always initial_suspend() { return {}; }

		suspend_always final_suspend() noexcept { return {}; }

		void unhandled_exception() { }

		const T* GetResult() const { return oResult; }

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

	DNTask(HandleType handle) : tHandle(handle){}

	void Resume()
	{
		if(!tHandle || tHandle.done())
		{
			return;
		}

		tHandle.resume();
	}

	void CallResume()
	{
		if(!pCallPause)
		{
			return;
		}

		pCallPause.resume();
	}

	T GetResult()
	{
		return *tHandle.promise().GetResult();
	}

	void Destroy()
	{
		if(tHandle)
		{
			tHandle.destroy();
		}
	}

	HandleType tHandle;
	coroutine_handle<> pCallPause;
};

export struct DNTaskVoid
{
	struct promise_type;
	using HandleType = coroutine_handle<promise_type>;
	struct promise_type
	{
		void return_void(){}

		DNTaskVoid get_return_object()
		{
			return DNTaskVoid{HandleType::from_promise(*this)};
		}

		suspend_never initial_suspend() { return {}; }

		suspend_never final_suspend() noexcept { return {}; }

		void unhandled_exception() {  }
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

	DNTaskVoid(HandleType handle): tHandle(handle){}

	void Resume()
	{
		if(!tHandle || tHandle.done())
		{
			return;
		}

		tHandle.resume();
	}

	HandleType tHandle;
};