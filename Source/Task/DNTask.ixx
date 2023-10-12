module;
#include <functional>
#include <coroutine>
export module DNTask;

export template <typename T>
struct DNTask
{
	struct promise_type
	{
		DNTask get_return_object()
		{
			return DNTask{std::coroutine_handle<promise_type>::from_promise(*this)};
		}

		void return_value(const T &value)
		{
			oResult = value;
		}
		auto initial_suspend() { return std::suspend_never{}; }

		auto final_suspend() noexcept { return std::suspend_always{}; }

		auto await_resume() noexcept { return std::suspend_always{}; }

		const T &getResult() const { return oResult; }

		void unhandled_exception() { std::terminate(); }

		T oResult;
	};

	DNTask(auto handle){tHandle = handle;}
	~DNTask(){if(tHandle)tHandle.destroy();}

	auto GetHandle() const { return tHandle; }

	std::coroutine_handle<promise_type> tHandle;
};