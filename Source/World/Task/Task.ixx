export module Task;

import std.compat;
import BitFlag;
import ThirdParty.Protobuf;

std::atomic<size_t> counter = 0;

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
		Task(const Task&) = delete;
    	Task& operator=(const Task&) = delete;

		struct promise_type;
		using HandleType = std::coroutine_handle<promise_type>;
		struct promise_type
		{
			promise_type()
			{
				iTaskId = ++counter;
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
			}

			~promise_type()
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
			}

			Task get_return_object()
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
				return Task{ HandleType::from_promise(*this) };
			}

			void return_value(T value)
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
				oResult = value;

				bIsDone = true;
				ReleaseAwaitHandle();
			}

			auto initial_suspend()
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
				// return std::suspend_always{};
				return std::suspend_never{}; 

			}

			auto final_suspend() noexcept
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
				// Task don't Call by self, need Message handle Tick;
				// return std::suspend_always{};
				return std::suspend_never{};
			}

			void unhandled_exception() {}

			void ReleaseAwaitHandle()
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
				if (pAwaitHandle) { pAwaitHandle.resume(); pAwaitHandle = nullptr; }
			}

			T oResult;
			bool bIsDone = false;

			std::coroutine_handle<> pAwaitHandle = nullptr;

			int iTaskId = 0;
		};

#pragma region Awaitable

		bool await_ready() const noexcept
		{
			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;

			return tHandle.promise().bIsDone;
		}

		void await_suspend(std::coroutine_handle<> caller)
		{
			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;
			tHandle.promise().pAwaitHandle = caller;
		}

		T await_resume() noexcept
		{
			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;
			return tHandle.promise().oResult;
		}

#pragma endregion

		Task(HandleType handle)
		{
			tHandle = handle;
			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;
			// SetFlag(EMTaskFlag::TimeCost);
		}

		~Task()
		{
			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;
			// Destroy();
		}

		// void Destroy()
		// {
		// 	if(tHandle)
		// 	{
		// 		if(tHandle.done())
		// 		{
		// 			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;
		// 			tHandle.destroy();
		// 			tHandle = nullptr;
		// 		}
		// 	}
		// }

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
		TaskVoid(const TaskVoid&) = delete;
    	TaskVoid& operator=(const TaskVoid&) = delete;

		struct promise_type;
		using HandleType = std::coroutine_handle<promise_type>;
		struct promise_type
		{
			promise_type()
			{
				iTaskId = ++counter;
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
			}

			~promise_type()
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
			}

			void return_void()
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
				bIsDone = true;
				ReleaseAwaitHandle();
			}

			TaskVoid get_return_object()
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
				return TaskVoid{ HandleType::from_promise(*this) };
			}

			// 返回suspend_never 表示无需挂起直接执行函数体
			auto initial_suspend()
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
				return std::suspend_never{};
			}

			auto final_suspend() noexcept
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
				return std::suspend_never{};
			}

			void unhandled_exception() {}

			void ReleaseAwaitHandle()
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;

				if (pAwaitHandle) 
				{ 
					pAwaitHandle.resume();
					pAwaitHandle = nullptr; 
				}
			}

			std::coroutine_handle<> pAwaitHandle = nullptr;

			bool bIsDone = false;

			int iTaskId = 0;
		};

#pragma region Awaitable Start

		// 根据有没有co_return 决定要不要挂起
		bool await_ready() const noexcept
		{
			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;

			return tHandle.promise().bIsDone;
		}

		void await_suspend(std::coroutine_handle<> caller)
		{
			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;
			tHandle.promise().pAwaitHandle = caller;
		}

		void await_resume() noexcept
		{
			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;
		}
#pragma endregion

		TaskVoid(HandleType handle)
		{
			tHandle = handle;
			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;
		}

		~TaskVoid()
		{
			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;
		}

		HandleType tHandle;

	};

	struct MsgTask : public BitFlag<EMTaskFlag>
	{
		MsgTask(const MsgTask&) = delete;
    	MsgTask& operator=(const MsgTask&) = delete;

		struct promise_type;
		using HandleType = std::coroutine_handle<promise_type>;
		struct promise_type
		{
			promise_type()
			{
				iTaskId = ++counter;
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
			}

			~promise_type()
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
			}

			MsgTask get_return_object()
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
				return MsgTask{ HandleType::from_promise(*this) };
			}

			void return_void()
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
				ReleaseAwaitHandle();
			}

			auto initial_suspend()
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
				return std::suspend_always{};
			}

			auto final_suspend() noexcept
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;
				return std::suspend_never{};
				// return std::suspend_always{};
			}

			void unhandled_exception() {}

			void ReleaseAwaitHandle()
			{
				std::cout << "TaskId:" << iTaskId <<  __FUNCTION__ << std::endl;

				if (pAwaitHandle) 
				{ 
					pAwaitHandle.resume();
					pAwaitHandle = nullptr; 
				}
			}

			std::coroutine_handle<> pAwaitHandle = nullptr;

			int iTaskId = 0;
		};

#pragma region Awaitable

		// 永远挂起
		bool await_ready() const noexcept
		{
			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;
			return false;
		}

		void await_suspend(std::coroutine_handle<> caller)
		{
			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;
			tHandle.promise().pAwaitHandle = caller;

			// if (HasFlag(EMTaskFlag::TimeCost))
			// {
			// 	oTimePoint = std::chrono::steady_clock::now();
			// }

			pCallback();

			// std::thread([this]() {
			// 	std::this_thread::sleep_for(std::chrono::seconds(5));
			// 	Resume();
			// }).detach();

		}

		void await_resume() noexcept
		{
			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;
		}

#pragma endregion

		MsgTask(HandleType handle)
		{
			tHandle = handle;
			
			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;
		}

		~MsgTask()
		{
			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;
			// Destroy();
		}

		// void Destroy()
		// {
		// 	if(tHandle)
		// 	{
		// 		if(tHandle.done())
		// 		{
		// 			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;
		// 			tHandle.destroy();
		// 			tHandle = nullptr;
		// 		}
		// 	}
		// }

		void SetCallback(std::function<void()>&& func)
		{
			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;
			pCallback = std::move(func);
		}

		void Resume()
		{
			std::cout << "TaskId:" << tHandle.promise().iTaskId <<  __FUNCTION__ << std::endl;
	
			tHandle.resume();
		}

	public:

		size_t GetTimerId() { return iTimerId; }

		void SetTimerId(size_t timerId) { iTimerId = timerId; }

		Message* GetMessage() { return pMessage; }

		void SetMessage(Message* message) { pMessage = message; }

	private:

		HandleType tHandle;

		size_t iTimerId = 0;

		// std::chrono::steady_clock::time_point oTimePoint;

		Message* pMessage = nullptr;

		std::function<void()> pCallback;
	};

	MsgTask MakeMsgTask()
	{
		co_return;
	}

}
