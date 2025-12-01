export module Task;

import std.compat;
import BitFlag;
import ThirdParty.Protobuf;

namespace
{
	std::atomic<size_t> counter = 0;
}

export
{
	/// @brief Task status flags
	enum class EMTaskFlag : uint16_t
	{
		None = 0,
		Timeout = 1,
		PaserError,
		TimeCost,
		Max,
	};

	/// @brief Awaitable task with result type
	template <typename T>
	struct Task : public BitFlag<EMTaskFlag>
	{
		using ResultType = std::conditional_t<std::is_reference_v<T>, 
			std::add_pointer_t<std::remove_reference_t<T>>, T>;

		// Non-copyable
		Task(const Task&) = delete;
		Task& operator=(const Task&) = delete;

		struct promise_type;
		using HandleType = std::coroutine_handle<promise_type>;

		struct promise_type
		{
			promise_type() = default;
			~promise_type() = default;

			[[nodiscard]] Task get_return_object() noexcept
			{
				return Task{ HandleType::from_promise(*this) };
			}

			template<typename U = T>
			requires (!std::is_void_v<U>)
			void return_value(U&& value)
			{
				oResult = std::forward<T>(value);
				bIsDone = true;
			}

			[[nodiscard]] auto initial_suspend() noexcept
			{
				return std::suspend_always{};
			}

			[[nodiscard]] auto final_suspend() noexcept
			{
				ReleaseAwaitHandle();
				return std::suspend_never{};
			}

			void unhandled_exception() noexcept
			{
				oResult = std::current_exception();
			}

			void ReleaseAwaitHandle() noexcept
			{
				if (pAwaitHandle) 
				{
					auto handle = std::exchange(pAwaitHandle, nullptr);
					handle.resume();
				}
			}

			std::variant<std::monostate, ResultType, std::exception_ptr> oResult;
			bool bIsDone = false;
			std::coroutine_handle<> pAwaitHandle = nullptr;
		};

		// Awaitable interface
		[[nodiscard]] bool await_ready() const noexcept
		{
			return tHandle.promise().bIsDone;
		}

		void await_suspend(std::coroutine_handle<> caller) noexcept
		{
			tHandle.promise().pAwaitHandle = caller;
			tHandle.resume();
		}

		T await_resume()
		{
			auto& promise = tHandle.promise();
			
			// Check for exception
			if (auto* ex = std::get_if<std::exception_ptr>(&promise.oResult)) [[unlikely]]
			{
				std::rethrow_exception(*ex);
			}

			if constexpr (std::is_reference_v<T>)
			{
				if (auto* ptr = std::get_if<ResultType>(&promise.oResult)) [[likely]]
				{
					return **ptr;
				}
			}
			else
			{
				if (auto* value = std::get_if<ResultType>(&promise.oResult)) [[likely]]
				{
					return std::move(*value);
				}
			}

			throw std::runtime_error("No result available");
		}

		explicit Task(HandleType handle) noexcept : tHandle(handle) {}

		~Task() = default;

		void Destroy() noexcept
		{
			if (tHandle && tHandle.done())
			{
				tHandle.destroy();
				tHandle = nullptr;
			}
		}

		void Resume() noexcept
		{
			if (tHandle && !tHandle.done())
			{
				tHandle.resume();
			}
		}

		[[nodiscard]] size_t GetTimerId() const noexcept { return iTimerId; }
		void SetTimerId(size_t timerId) noexcept { iTimerId = timerId; }

	private:
		HandleType tHandle;
		size_t iTimerId = 0;
		[[maybe_unused]] std::chrono::steady_clock::time_point oTimePoint;
	};


	/// @brief Void task for fire-and-forget coroutines
	struct TaskVoid
	{
		TaskVoid(const TaskVoid&) = delete;
		TaskVoid& operator=(const TaskVoid&) = delete;

		struct promise_type;
		using HandleType = std::coroutine_handle<promise_type>;

		struct promise_type
		{
			promise_type() = default;
			~promise_type() = default;

			void return_void() noexcept { bIsDone = true; }

			[[nodiscard]] TaskVoid get_return_object() noexcept
			{
				return TaskVoid{ HandleType::from_promise(*this) };
			}

			[[nodiscard]] auto initial_suspend() noexcept { return std::suspend_never{}; }

			[[nodiscard]] auto final_suspend() noexcept
			{
				ReleaseAwaitHandle();
				return std::suspend_never{};
			}

			void unhandled_exception() noexcept {}

			void ReleaseAwaitHandle() noexcept
			{
				if (pAwaitHandle) 
				{ 
					auto handle = std::exchange(pAwaitHandle, nullptr);
					handle.resume();
				}
			}

			std::coroutine_handle<> pAwaitHandle = nullptr;
			bool bIsDone = false;
			[[maybe_unused]] bool bHasAwaited = false;
		};

		// Awaitable interface
		[[nodiscard]] bool await_ready() const noexcept
		{
			return tHandle.promise().bIsDone;
		}

		void await_suspend(std::coroutine_handle<> caller) noexcept
		{
			tHandle.promise().pAwaitHandle = caller;
		}

		void await_resume() noexcept {}

		explicit TaskVoid(HandleType handle) noexcept : tHandle(handle) {}

		~TaskVoid() = default;

		void Destroy() noexcept
		{
			if (tHandle && tHandle.done())
			{
				tHandle.destroy();
				tHandle = nullptr;
			}
		}

		HandleType tHandle;
	};

	/// @brief Message task for async message handling
	struct MsgTask : public BitFlag<EMTaskFlag>
	{
		MsgTask(const MsgTask&) = delete;
		MsgTask& operator=(const MsgTask&) = delete;

		struct promise_type;
		using HandleType = std::coroutine_handle<promise_type>;

		struct promise_type
		{
			promise_type() = default;
			~promise_type() = default;

			[[nodiscard]] MsgTask get_return_object() noexcept
			{
				return MsgTask{ HandleType::from_promise(*this) };
			}

			void return_void() noexcept {}

			[[nodiscard]] auto initial_suspend() noexcept { return std::suspend_always{}; }

			[[nodiscard]] auto final_suspend() noexcept
			{
				ReleaseAwaitHandle();
				return std::suspend_never{};
			}

			void unhandled_exception() noexcept {}

			void ReleaseAwaitHandle() noexcept
			{
				if (pAwaitHandle) 
				{ 
					auto handle = std::exchange(pAwaitHandle, nullptr);
					handle.resume();
				}
			}

			std::coroutine_handle<> pAwaitHandle = nullptr;
		};

		// Awaitable interface - always suspends
		[[nodiscard]] bool await_ready() const noexcept { return false; }

		void await_suspend(std::coroutine_handle<> caller)
		{
			tHandle.promise().pAwaitHandle = caller;
			pCallback();
		}

		void await_resume() noexcept {}

		explicit MsgTask(HandleType handle) noexcept : tHandle(handle) {}

		~MsgTask() = default;

		void SetCallback(std::function<void()>&& func) noexcept
		{
			pCallback = std::move(func);
		}

		void Resume() noexcept
		{
			if (tHandle) [[likely]]
			{
				tHandle.resume();
			}
		}

		void Destroy() noexcept
		{
			if (tHandle && tHandle.done())
			{
				tHandle.destroy();
			}
			tHandle = nullptr;
		}

		[[nodiscard]] size_t GetTimerId() const noexcept { return iTimerId; }
		void SetTimerId(size_t timerId) noexcept { iTimerId = timerId; }

		[[nodiscard]] Message* GetMessage() noexcept { return pMessage; }
		void SetMessage(Message* message) noexcept { pMessage = message; }

	private:
		HandleType tHandle;
		size_t iTimerId = 0;
		Message* pMessage = nullptr;
		std::function<void()> pCallback;
	};

	/// @brief Factory function for creating MsgTask
	[[nodiscard]] inline MsgTask MakeMsgTask()
	{
		co_return;
	}

}
