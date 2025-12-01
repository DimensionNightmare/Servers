export module FuncUtils;

import std;
import StrUtils;

export
{

#pragma region FunctionTraits

	/// @brief Type traits for function types
	template<typename T>
	struct FunctionTraits;

	/// @brief Specialization for function pointers
	template<typename Ret, typename... Args>
	struct FunctionTraits<Ret(*)(Args...)>
	{
		using ReturnType = Ret;
		using ArgType = std::tuple<std::decay_t<Args>...>;
		using ArgTypeForaward = std::tuple<Args&&...>;
		
		static constexpr bool IsMemberFunction = false;
	};

	/// @brief Specialization for member function pointers
	template<typename Ret, typename Class, typename... Args>
	struct FunctionTraits<Ret(Class::*)(Args...)>
	{
		using ReturnType = Ret;
		using ArgType = std::tuple<std::decay_t<Args>...>;
		using ArgTypeForaward = std::tuple<Args&&...>;

		using ClassType = Class;

		static constexpr bool IsMemberFunction = true;
	};

	/// @brief Specialization for std::function
	template<typename Ret, typename... Args>
	struct FunctionTraits<std::function<Ret(Args...)>>
	{
		using ReturnType = Ret;
		using ArgType = std::tuple<std::decay_t<Args>...>;
		using ArgTypeForaward = std::tuple<Args&&...>;

		using FuncSign = std::function<Ret(Args...)>;
		
		static constexpr bool IsMemberFunction = false;
	};
	
#pragma endregion

#pragma region EventContainer
	
	/// @brief Interface for event containers using virtual dispatch
	struct IEventContainer
	{
		size_t mTypeHash = 0;

#if DN_DEBUG_EVENT
		std::string_view sTypeSign;
#endif

		virtual ~IEventContainer() = default;
		virtual bool Invoke(void* param) = 0;
	};

	template<auto Func>
	class EventContainer;

	template<typename Func>
	class DynamicEventContainer;

	/// @brief Event container for static function pointers
	template<typename Ret, typename... Args, Ret(*Func)(Args...)>
	class EventContainer<Func> final : public IEventContainer
	{
		using Traits = FunctionTraits<decltype(Func)>;
		using ArgType = typename Traits::ArgType;
		using ArgTypeForaward = typename Traits::ArgTypeForaward;

		std::function<Ret(Args...)> mProxy;

	public:
		EventContainer()
		{
			constexpr auto typeHash = TupleTypeHash<ArgType>();
			mTypeHash = typeHash;

#if DN_DEBUG_EVENT
			constexpr auto typeSign = TupleTypeStr<ArgType>();
			sTypeSign = typeSign;
#endif
			mProxy = [](auto&&... args) -> Ret
			{
				return Func(std::forward<decltype(args)>(args)...);
			};
		}

		~EventContainer() override = default;

		Ret operator()(auto&&... args) const
		{
			return mProxy(std::forward<decltype(args)>(args)...);
		}

		bool Invoke(void* param) override
		{
			auto* tuplePtr = static_cast<ArgTypeForaward*>(param);

			std::apply([this](auto&&... args)
			{
				(*this)(std::forward<decltype(args)>(args)...);
			}, *tuplePtr);

			return true;
		}
	};

	/// @brief Event container for member function pointers
	template<typename Ret, typename Class, typename... Args, Ret(Class::* Func)(Args...)>
	class EventContainer<Func> final : public IEventContainer
	{
		using Traits = FunctionTraits<decltype(Func)>;
		using ArgType = typename Traits::ArgType;
		using ArgTypeForaward = typename Traits::ArgTypeForaward;

	private:
		std::weak_ptr<Class> pInstance;
		Class* pInstanceOrigin = nullptr;
		std::function<Ret(Class*, Args...)> mProxy;

	public:
		EventContainer()
		{
			constexpr auto typeHash = TupleTypeHash<ArgType>();
			mTypeHash = typeHash;

#if DN_DEBUG_EVENT
			constexpr auto typeSign = TupleTypeStr<ArgType>();
			sTypeSign = typeSign;
#endif
			mProxy = [](Class* obj, auto&&... args) -> Ret
			{
				return (obj->*Func)(std::forward<decltype(args)>(args)...);
			};
		}

		explicit EventContainer(Class* instance) : EventContainer()
		{
			pInstanceOrigin = instance;
		}
	
		explicit EventContainer(const std::shared_ptr<Class>& instance) : EventContainer()
		{
			pInstance = instance;
		}

		~EventContainer() override = default;

		Ret operator()(auto&&... args) const
		{
			if (pInstanceOrigin) [[likely]]
			{
				return mProxy(pInstanceOrigin, std::forward<decltype(args)>(args)...);
			}

			if (auto instance = pInstance.lock()) [[likely]]
			{
				return mProxy(instance.get(), std::forward<decltype(args)>(args)...);
			}
			
			throw std::runtime_error("Tick EventContainer Error");
		}

		[[nodiscard]] bool IsValid() const noexcept
		{
			return pInstanceOrigin != nullptr || !pInstance.expired();
		}

		bool Invoke(void* param) override
		{
			auto* tuplePtr = static_cast<ArgTypeForaward*>(param);

			std::apply([this](auto&&... args)
			{
				if (IsValid()) [[likely]]
				{
					(*this)(std::forward<decltype(args)>(args)...);
				}
			}, *tuplePtr);

			return true;
		}
	};

	/// @brief Event container for dynamic function objects
	template<typename Func>
	class DynamicEventContainer final : public IEventContainer
	{
	private:
		Func mProxy;

		using Traits = FunctionTraits<Func>;
		using Ret = typename Traits::ReturnType;
		using ArgType = typename Traits::ArgType;
		using ArgTypeForaward = typename Traits::ArgTypeForaward;

	public:
		explicit DynamicEventContainer(Func* func) : mProxy(std::move(*func))
		{
			constexpr auto typeHash = TupleTypeHash<ArgType>();
			mTypeHash = typeHash;

#if DN_DEBUG_EVENT
			constexpr auto typeSign = TupleTypeStr<ArgType>();
			sTypeSign = typeSign;
#endif
		}

		~DynamicEventContainer() override = default;

		Ret operator()(auto&&... args) const
		{
			return mProxy(std::forward<decltype(args)>(args)...);
		}

		[[nodiscard]] bool IsValid() const noexcept 
		{ 
			return static_cast<bool>(mProxy); 
		}

		bool Invoke(void* param) override
		{
			auto* tuplePtr = static_cast<ArgTypeForaward*>(param);

			std::apply([this](auto&&... args)
			{
				if (IsValid()) [[likely]]
				{
					(*this)(std::forward<decltype(args)>(args)...);
				}
			}, *tuplePtr);

			return true;
		}
	};

#pragma endregion

}

/// @brief CRTP helper class for type-safe inheritance
export template <typename Derived, typename Base>
class Helper : public Base
{
public:
	using Base::Base;
	using Ptr = std::shared_ptr<Derived>;
	using CVPtr = const Ptr&;
	
	// Delete all copy/move operations and memory management
	Helper() = delete;
	Helper(Helper&) = delete;
	Helper(const Helper&) = delete;
	Helper(Helper&&) = delete;
	~Helper() = default;
	
	Helper& operator=(const Helper&) = delete;
	Helper& operator=(Helper&&) = delete;
	
	void* operator new(size_t) = delete;
	void operator delete(void*) = delete;
	
	[[nodiscard]] Base* GetBase() noexcept
	{
		return static_cast<Base*>(this);
	}

	[[nodiscard]] const Base* GetBase() const noexcept
	{
		return static_cast<const Base*>(this);
	}
};
