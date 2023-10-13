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

	DNTask()
	{
		memset(this, 0, sizeof *this);
	}

	DNTask(auto handle)
	{
		memset(this, 0, sizeof *this);
		tHandle = handle;
	}

	~DNTask()
	{
		if(tHandle)tHandle.destroy();
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

	void SetResult(T& res)
	{
		tHandle.promise().return_value(res);
		if(pCaller)
		{
        	pCaller.resume();
			pCaller = nullptr;
		}
	}

	auto GetHandle()
	{
		return tHandle;
	}

private:
	coroutine_handle<> pCaller;
	coroutine_handle<promise_type> tHandle;
};

export struct DNTaskVoid
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

		void return_void(){}

		auto get_return_object()
		{
			return DNTaskVoid{coroutine_handle<promise_type>::from_promise(*this)};
		}

		auto initial_suspend() { return suspend_never{}; }

		auto final_suspend() noexcept { return suspend_always{}; }

		void unhandled_exception() { terminate(); }
	};

	DNTaskVoid(auto handle)
	{
		memset(this, 0, sizeof *this);
		tHandle = handle;
	}

	~DNTaskVoid()
	{
		if(tHandle)tHandle.destroy();
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

private:
	coroutine_handle<promise_type> tHandle;
};