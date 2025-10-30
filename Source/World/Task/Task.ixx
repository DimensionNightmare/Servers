export module Task;

import std.compat;
import BitFlag;

using namespace std::chrono;

export
{
	
	enum class EMTaskFlag : uint16_t
	{
		None = 0,
		Timeout = 1,
		PaserError,
		TimeCost,
		Max,
	
	};

	template <typename T>
	struct Task : public BitFlag<EMTaskFlag>
	{
		struct promise_type;
		using HandleType = std::coroutine_handle<promise_type>;
		struct promise_type
		{
			promise_type()
			{
			}
	
			Task get_return_object()
			{
				return Task{ HandleType::from_promise(*this) };
			}
	
			void return_value(const T& value)
			{
				oResult = &value;
				bReturned = true;
			}
	
			std::suspend_always initial_suspend() { return {}; }
	
			std::suspend_always final_suspend() noexcept
			{
				// Task don't Call by self, need Message handle Tick;
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
	
			std::coroutine_handle<> oAwaitHandle = nullptr;
	
			bool bReturned = false;
		};
	
	#pragma region Awaitable
	
		bool await_ready() const noexcept
		{
			return tHandle.promise().bReturned;
		}
	
		void await_suspend(std::coroutine_handle<> caller)
		{
			tHandle.promise().oAwaitHandle = caller;
	
			if (HasFlag(EMTaskFlag::TimeCost))
			{
				oTimePoint = steady_clock::now();
			}
		}
	
		void await_resume() noexcept
		{
		}
	#pragma endregion
	
		Task(HandleType handle)
		{
			tHandle = handle;
			// SetFlag(EMTaskFlag::TimeCost);
		}
	
		~Task()
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
			if (HasFlag(EMTaskFlag::TimeCost))
			{
				// steady_clock::time_point now = steady_clock::now();
				// LoggerPrint::Log(nullptr, ELogLevel_Normal, "tasktimeid:{}, cost:{}ms", iTimerId, duration_cast<microseconds>(now - oTimePoint).count() / 1000.0);
			}
	
			if (tHandle)
			{
				tHandle.destroy();
				tHandle = nullptr;
			}
		}
	public:
	
		size_t GetTimerId() { return iTimerId; }

		void SetTimerId(size_t timerId) { iTimerId = timerId; }
	
	private:
	
		HandleType tHandle;
	
		size_t iTimerId = 0;
	
		steady_clock::time_point oTimePoint;
	};
	
	struct TaskVoid
	{
		struct promise_type;
		using HandleType = std::coroutine_handle<promise_type>;
		struct promise_type
		{
			promise_type() {}
	
			void return_void() { bReturned = true; }
	
			TaskVoid get_return_object()
			{
				return TaskVoid{ HandleType::from_promise(*this) };
			}
	
			std::suspend_never initial_suspend() { return {}; }
	
			std::suspend_never final_suspend() noexcept
			{
				ReleaseAwaitHandle();
				return {};
			}
	
			void unhandled_exception() {}
	
			void ReleaseAwaitHandle()
			{
				if (oAwaitHandle) { oAwaitHandle.resume(); oAwaitHandle = nullptr; }
			}
	
			std::coroutine_handle<> oAwaitHandle = nullptr;
	
			bool bReturned = false;
		};
	
	#pragma region Awaitable Start
	
		bool await_ready() const noexcept
		{
			return tHandle.promise().bReturned;
		}
	
		void await_suspend(std::coroutine_handle<> caller)
		{
			tHandle.promise().oAwaitHandle = caller;
		}
	
		void await_resume() noexcept
		{
		}
	#pragma endregion
	
		TaskVoid(HandleType handle)
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
	
}
