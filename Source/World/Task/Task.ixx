export module Task;

import std.compat;
import BitFlag;
import ThirdParty.Protobuf;

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
				std::cout <<  __FUNCTION__ << "\n";
			}

			~promise_type()
			{
				std::cout <<  __FUNCTION__ << "\n";
				ReleaseAwaitHandle();
			}

			Task get_return_object()
			{
				std::cout <<  __FUNCTION__ << "\n";
				return Task{ HandleType::from_promise(*this) };
			}

			void return_value(const T& value)
			{
				std::cout <<  __FUNCTION__ << "\n";
				oResult = &value;
			}

			auto initial_suspend()
			{
				std::cout <<  __FUNCTION__ << "\n";
				// return std::suspend_always{};
				return std::suspend_never{}; 

			}

			auto final_suspend() noexcept
			{
				std::cout <<  __FUNCTION__ << "\n";
				// Task don't Call by self, need Message handle Tick;
				return std::suspend_never{};
			}

			void unhandled_exception() {}

			void ReleaseAwaitHandle()
			{
				std::cout <<  __FUNCTION__ << "\n";
				if (pAwaitHandle) { pAwaitHandle.resume(); pAwaitHandle = nullptr; }
			}

			const T* oResult = nullptr;

			std::coroutine_handle<> pAwaitHandle = nullptr;
		};

#pragma region Awaitable

		bool await_ready() const noexcept
		{
			std::cout <<  __FUNCTION__ << "\n";
			return false;
		}

		void await_suspend(std::coroutine_handle<> caller)
		{
			std::cout <<  __FUNCTION__ << "\n";
			tHandle.promise().pAwaitHandle = caller;

			if (HasFlag(EMTaskFlag::TimeCost))
			{
				oTimePoint = std::chrono::steady_clock::now();
			}

			// tHandle.resume();
			
		}

		const T& await_resume() noexcept
		{
			return GetResult();
		}

#pragma endregion

		Task(HandleType handle)
		{
			std::cout <<  __FUNCTION__ << "\n";
			tHandle = handle;
			// SetFlag(EMTaskFlag::TimeCost);
		}

		~Task()
		{
			std::cout <<  __FUNCTION__ << "\n";
			Destroy();
		}

		const T& GetResult()
		{
			std::cout <<  __FUNCTION__ << "\n";
			const T* result = tHandle.promise().oResult;

			return *result;
		}

		void Destroy()
		{
			std::cout <<  __FUNCTION__ << "\n";
			if (HasFlag(EMTaskFlag::TimeCost))
			{
				// steady_clock::time_point now = steady_clock::now();
				// LoggerPrint::Log(nullptr, ELogLevel_Normal, "tasktimeid:{}, cost:{}ms", iTimerId, duration_cast<microseconds>(now - oTimePoint).count() / 1000.0);
			}

			if (tHandle && tHandle.done())
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

		std::chrono::steady_clock::time_point oTimePoint;
	};


	struct TaskVoid
	{
		struct promise_type;
		using HandleType = std::coroutine_handle<promise_type>;
		struct promise_type
		{
			promise_type()
			{
				std::cout <<  __FUNCTION__ << "\n";
			}

			~promise_type()
			{
				std::cout <<  __FUNCTION__ << "\n";
				ReleaseAwaitHandle();
			}

			void return_void()
			{
				std::cout <<  __FUNCTION__ << "\n";
			}

			TaskVoid get_return_object()
			{
				std::cout <<  __FUNCTION__ << "\n";
				return TaskVoid{ HandleType::from_promise(*this) };
			}

			auto initial_suspend()
			{
				std::cout <<  __FUNCTION__ << "\n";
				// return std::suspend_always{};
				return std::suspend_never{}; 
			}

			auto final_suspend() noexcept
			{
				std::cout <<  __FUNCTION__ << "\n";
				return std::suspend_never{};
				// return std::suspend_always{}; 
			}

			void unhandled_exception() {}

			void ReleaseAwaitHandle()
			{
				std::cout <<  __FUNCTION__ << "\n";
				if (pAwaitHandle) 
				{ 
					pAwaitHandle.resume();
					pAwaitHandle = nullptr; 
				}
			}

			std::coroutine_handle<> pAwaitHandle = nullptr;
		};

#pragma region Awaitable Start

		bool await_ready() const noexcept
		{
			std::cout <<  __FUNCTION__ << "\n";
			return true;
		}

		void await_suspend(std::coroutine_handle<> caller)
		{
			std::cout <<  __FUNCTION__ << "\n";
			tHandle.promise().pAwaitHandle = caller;
			// tHandle.resume();
		}

		void await_resume() noexcept
		{
			std::cout <<  __FUNCTION__ << "\n";
			// if (tHandle && !tHandle.done())
			// {
			// 	tHandle.resume();
			// }
		}
#pragma endregion

		TaskVoid(HandleType handle)
		{
			std::cout <<  __FUNCTION__ << "\n";
			tHandle = handle;
		}

		~TaskVoid()
		{
			std::cout <<  __FUNCTION__ << "\n";
			Destroy();
		}

		void Destroy()
		{
			std::cout <<  __FUNCTION__ << "\n";
			if (tHandle && tHandle.done())
			{
				tHandle.destroy();
				tHandle = nullptr;
			}
		}

		HandleType tHandle;
	};

	struct MsgTask : public BitFlag<EMTaskFlag>
	{
		struct promise_type;
		using HandleType = std::coroutine_handle<promise_type>;
		struct promise_type
		{
			promise_type()
			{
				std::cout <<  __FUNCTION__ << "\n";
			}

			~promise_type()
			{
				std::cout <<  __FUNCTION__ << "\n";
			}

			MsgTask get_return_object()
			{
				std::cout <<  __FUNCTION__ << "\n";
				return MsgTask{ HandleType::from_promise(*this) };
			}

			void return_void()
			{
				std::cout <<  __FUNCTION__ << "\n";
			}

			auto initial_suspend()
			{
				std::cout <<  __FUNCTION__ << "\n";
				return std::suspend_always{};
			}

			auto final_suspend() noexcept
			{
				std::cout <<  __FUNCTION__ << "\n";
				return std::suspend_never{};
			}

			void unhandled_exception() {}

			std::coroutine_handle<> pAwaitHandle = nullptr;
		};

#pragma region Awaitable

		bool await_ready() const noexcept
		{
			std::cout <<  __FUNCTION__ << "\n";
			return false;
		}

		void await_suspend(std::coroutine_handle<> caller)
		{
			std::cout <<  __FUNCTION__ << "\n";
			tHandle.promise().pAwaitHandle = caller;

			if (HasFlag(EMTaskFlag::TimeCost))
			{
				oTimePoint = std::chrono::steady_clock::now();
			}

			pCallback();
		}

		void await_resume() noexcept
		{
			std::cout <<  __FUNCTION__ << "\n";
			tHandle.resume();
		}

#pragma endregion

		MsgTask(HandleType handle)
		{
			std::cout <<  __FUNCTION__ << "\n";
			tHandle = handle;
			// SetFlag(EMTaskFlag::TimeCost);
		}

		~MsgTask()
		{
			std::cout <<  __FUNCTION__ << "\n";
			Destroy();
		}

		void SetCallback(std::function<void()>&& func)
		{
			std::cout <<  __FUNCTION__ << "\n";
			pCallback = std::move(func);
		}

		void CallResume()
		{
			std::cout <<  __FUNCTION__ << "\n";
			// if(tHandle && !tHandle.done())
			// {
			// 	return;
			// }
			tHandle.promise().pAwaitHandle.resume();
		}

		void Destroy()
		{
			std::cout <<  __FUNCTION__ << "\n";
			if (HasFlag(EMTaskFlag::TimeCost))
			{
				// steady_clock::time_point now = steady_clock::now();
				// LoggerPrint::Log(nullptr, ELogLevel_Normal, "tasktimeid:{}, cost:{}ms", iTimerId, duration_cast<microseconds>(now - oTimePoint).count() / 1000.0);
			}
			
			if (tHandle && tHandle.done())
			{
				tHandle.destroy();
				tHandle = nullptr;
			}
		}
	public:

		size_t GetTimerId() { return iTimerId; }

		void SetTimerId(size_t timerId) { iTimerId = timerId; }

		Message* GetMessage() { return pMessage; }

		void SetMessage(Message* message) { pMessage = message; }

	private:

		HandleType tHandle;

		size_t iTimerId = 0;

		std::chrono::steady_clock::time_point oTimePoint;

		Message* pMessage = nullptr;

		std::function<void()> pCallback;
	};

	MsgTask MakeMsgTask()
	{
		co_return;
	}

}
