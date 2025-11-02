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

	template <typename T, bool BeginWait = false>
	struct Task : public BitFlag<EMTaskFlag>
	{
		struct promise_type;
		using HandleType = std::coroutine_handle<promise_type>;
		struct promise_type
		{
			promise_type()
			{
			}

			~promise_type()
			{
				ReleaseAwaitHandle();
			}

			Task get_return_object()
			{
				return Task{ HandleType::from_promise(*this) };
			}

			void return_value(const T& value)
			{
				oResult = &value;
			}

			auto initial_suspend()
			{
				return std::suspend_always{};
			}

			auto final_suspend() noexcept
			{
				// Task don't Call by self, need Message handle Tick;
				return std::suspend_never{};
			}

			void unhandled_exception() {}

			void ReleaseAwaitHandle()
			{
				if (pAwaitHandle) { pAwaitHandle.resume(); pAwaitHandle = nullptr; }
			}

			const T* oResult = nullptr;

			std::coroutine_handle<> pAwaitHandle = nullptr;
		};

#pragma region Awaitable

		bool await_ready() const noexcept
		{
			return false;
		}

		void await_suspend(std::coroutine_handle<> caller)
		{
			tHandle.promise().pAwaitHandle = caller;

			if (HasFlag(EMTaskFlag::TimeCost))
			{
				oTimePoint = std::chrono::steady_clock::now();
			}

			if constexpr (!BeginWait)
			{
				tHandle.resume();
			}
			
		}

		const T& await_resume() noexcept
		{
			return GetResult();
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

		Task(const Task&) = delete;
		Task& operator=(const Task&) = delete;

		const T& GetResult()
		{
			const T* result = tHandle.promise().oResult;

			return *result;
		}

		void Destroy()
		{
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
			}

			~promise_type()
			{
				ReleaseAwaitHandle();
			}

			void return_void()
			{
			}

			TaskVoid get_return_object()
			{
				return TaskVoid{ HandleType::from_promise(*this) };
			}

			auto initial_suspend()
			{
				return std::suspend_never{};
				// return std::suspend_always{}; 
			}

			auto final_suspend() noexcept
			{
				return std::suspend_never{};
				// return std::suspend_always{}; 
			}

			void unhandled_exception() {}

			void ReleaseAwaitHandle()
			{
				if (pAwaitHandle) { pAwaitHandle.resume(); pAwaitHandle = nullptr; }
			}

			std::coroutine_handle<> pAwaitHandle = nullptr;
		};

#pragma region Awaitable Start

		bool await_ready() const noexcept
		{
			return false;
		}

		void await_suspend(std::coroutine_handle<> caller)
		{
			tHandle.promise().pAwaitHandle = caller;
		}

		void await_resume() noexcept
		{
		}
#pragma endregion

		TaskVoid(HandleType handle)
		{
			tHandle = handle;
		}

		~TaskVoid()
		{
			Destroy();
		}

		void Destroy()
		{
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
			}

			~promise_type()
			{
			}

			MsgTask get_return_object()
			{
				return MsgTask{ HandleType::from_promise(*this) };
			}

			void return_void()
			{
			}

			auto initial_suspend()
			{
				return std::suspend_always{};
			}

			auto final_suspend() noexcept
			{
				return std::suspend_never{};
			}

			void unhandled_exception() {}

			std::coroutine_handle<> pAwaitHandle = nullptr;
		};

#pragma region Awaitable

		bool await_ready() const noexcept
		{
			return false;
		}

		void await_suspend(std::coroutine_handle<> caller)
		{
			tHandle.promise().pAwaitHandle = caller;

			if (HasFlag(EMTaskFlag::TimeCost))
			{
				oTimePoint = std::chrono::steady_clock::now();
			}
		}

		void await_resume() noexcept
		{
			tHandle.resume();
		}

#pragma endregion

		MsgTask(HandleType handle)
		{
			tHandle = handle;
			// SetFlag(EMTaskFlag::TimeCost);
		}

		~MsgTask()
		{
			Destroy();
		}

		void CallResume()
		{
			tHandle.promise().pAwaitHandle.resume();
		}

		void Destroy()
		{
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
	};

	MsgTask MakeMsgTask()
	{
		co_return;
	}

}
