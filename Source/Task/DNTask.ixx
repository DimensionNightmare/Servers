module;
#include <iostream>
#include <functional>
#include <coroutine>
export module DNTask;

using namespace std;

export template <typename T>
struct DNTask
{
	struct promise_type;
   	using handle_type = std::coroutine_handle<promise_type>;
	struct promise_type
	{
		~promise_type()
		{
			cout << "DNTask::promise_type clear" <<endl;
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

	DNTask() : tHandle(nullptr) {}

	DNTask(handle_type handle)
	{
		tHandle = handle;
	}

	~DNTask()
	{
		cout << "DNTask clear" <<endl;
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
	handle_type tHandle;
};

export struct DNTaskVoid
{
	struct promise_type
	{
		~promise_type()
		{
			cout << "DNTaskVoid::promise_type clear" <<endl;
		}
		void return_void(){}

		auto get_return_object()
		{
			return DNTaskVoid{coroutine_handle<promise_type>::from_promise(*this)};
		}

		auto initial_suspend() { return suspend_never{}; }

		auto final_suspend() noexcept { return suspend_never{}; }

		void unhandled_exception() { terminate(); }
	};

	DNTaskVoid(auto handle)
	{
		tHandle = handle;
	}

	DNTaskVoid() : tHandle(nullptr) {}

	~DNTaskVoid()
	{
		cout << "DNTaskVoid clear" <<endl;
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