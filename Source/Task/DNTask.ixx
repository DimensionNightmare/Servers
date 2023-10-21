module;
#include <iostream>
#include <functional>
#include <coroutine>
export module DNTask;

using namespace std;

export template <typename T>
struct DNTask
{
	struct promise_type
	{
		promise_type()
		{
			memset(this, 0, sizeof *this);
		}

		~promise_type()
		{
		}

		auto get_return_object()
		{
			return DNTask{coroutine_handle<promise_type>::from_promise(*this)};
		}

		void return_value(const T& value)
		{
			oResult = &value;
		}
		// 
		auto initial_suspend() { return suspend_always{}; }

		auto final_suspend() noexcept { return suspend_always{}; }

		void unhandled_exception() { terminate(); }

		const T* GetResult() const { return oResult; }

		const T* oResult;
	};

	DNTask(auto handle)
	{
		tHandle = handle;
	}

	constexpr bool await_ready() const noexcept 
	{
		if(tHandle.done())
			return true; 
		else
			return false;
	}

	void await_suspend(coroutine_handle<> caller) 
	{
		if(caller)
		{
			pCaller = caller;
		}
	}

	auto await_resume() noexcept 
	{
		return GetResult(); 
	}

	void Resume(){
		if(!tHandle || tHandle.done())
			return;

		tHandle.resume();
	}

	auto GetResult()
	{
		return *tHandle.promise().GetResult();
	}

	void CallResume()
	{
		if(pCaller)
		{
        	pCaller.resume();
			pCaller = nullptr;
		}
	}

	coroutine_handle<> pCaller;
	coroutine_handle<promise_type> tHandle;
};

export struct DNTaskVoid
{
	struct promise_type
	{
		void return_void(){}

		auto get_return_object()
		{
			return DNTaskVoid{coroutine_handle<promise_type>::from_promise(*this)};
		}

		auto initial_suspend() { return suspend_always{}; }

		auto final_suspend() noexcept { return suspend_always{}; }

		void unhandled_exception() { terminate(); }
	};

	DNTaskVoid(auto handle)
	{
		tHandle = handle;
	}

	constexpr bool await_ready() const noexcept 
	{
		return false;
	}

	void await_suspend(coroutine_handle<> caller) 
	{

	}

	void await_resume() noexcept 
	{
		 
	}

	void Resume(){
		if(!tHandle || tHandle.done())
			return;

		tHandle.resume();
	}

	coroutine_handle<promise_type> tHandle;
};