export module FuncUtils;

import std;

export
{
#pragma region FunctionContainer 
	template <auto Func>
	struct FunctionContainer;

	template <typename Ret, typename... Args, Ret(*Func)(Args...)>
	struct FunctionContainer<Func>
	{
	public:
		FunctionContainer()
		{
			mProxy = [this](Args... args) -> Ret
				{
					return (*Func)(std::forward<Args>(args)...);
				};
		}

		Ret operator()(Args... args) const
		{
			return mProxy(std::forward<Args>(args)...);
		}

		std::function<Ret(Args...)> mProxy;
	};

	template <typename Ret, typename Class, typename... Args, Ret(Class::* Func)(Args...)>
	struct FunctionContainer<Func>
	{
	public:
		FunctionContainer(Class* instance)
		{
			pInstance = instance;
			mProxy = [](Class* obj, Args&&... args) -> Ret
				{
					return (obj->*Func)(std::forward<Args>(args)...);
				};
		}

		Ret operator()(Args... args) const
		{
			if (!pInstance)
			{
				throw std::runtime_error("Instance not set for member function");
			}
			return mProxy(pInstance, std::forward<Args>(args)...);
		}

	private:
		Class* pInstance = nullptr;
		std::function<Ret(Class*, Args...)> mProxy;
	};

#pragma endregion

#pragma region FunctionTraits

	template<typename T>
	struct FunctionTraits;

	template<typename Ret, typename... Args>
	struct FunctionTraits<Ret(*)(Args...)>
	{
		using ReturnType = Ret;
		using ClassType = void;
		static constexpr bool IsMemberFunction = false;
		static constexpr size_t Arity = sizeof...(Args);

		using ArgType = std::tuple<std::decay_t<Args>...>;
	};

	template<typename Ret, typename Class, typename... Args>
	struct FunctionTraits<Ret(Class::*)(Args...)>
	{
		using ReturnType = Ret;
		using ClassType = Class;
		static constexpr bool IsMemberFunction = true;
		static constexpr size_t Arity = sizeof...(Args);

		using ArgType = std::tuple<std::decay_t<Args>...>;
	};

	template<typename Ret, typename... Args>
	struct FunctionTraits<std::function<Ret(Args...)>>
	{
		using ReturnType = Ret;
		using ClassType = void;
		static constexpr bool IsMemberFunction = false;
		static constexpr size_t Arity = sizeof...(Args);

		using ArgType = std::tuple<std::decay_t<Args>...>;
		using ArgTypeOrigin = std::tuple<Args&&...>;
	};
	
#pragma endregion

#pragma region EventContainer
	
	struct IEventContainer
	{
		size_t mTypeHash = 0;

#if DN_DEBUG_EVENT
		std::string_view sTypeSign;
#endif

		virtual bool Invoke(void* param) = 0;
	};

	template<auto Func>
	class EventContainer;


	template<typename Ret, typename... Args, Ret(*Func)(Args...)>
	class EventContainer<Func>
	{
	private:
		std::function<Ret(Args...)> mProxy;

	public:
	
		EventContainer()
		{
			mProxy = [](Args... args) -> Ret
				{
					return Func(std::forward<Args>(args)...);
				};
		}

		EventContainer(std::nullptr_t) : EventContainer() {} // 兼容性构造函数

		Ret operator()(Args... args) const
		{
			return mProxy(std::forward<Args>(args)...);
		}

		bool IsValid() const { return true; }
	};

	template<typename Ret, typename Class, typename... Args, Ret(Class::* Func)(Args...)>
	class EventContainer<Func> : public IEventContainer
	{
	private:
		std::weak_ptr<Class> pInstance;
		std::function<Ret(Class*, Args...)> mProxy;

	public:
	
		EventContainer(std::weak_ptr<Class> instance) : pInstance(instance)
		{
			mProxy = [](Class* obj, Args&&... args) -> Ret
				{
					return (obj->*Func)(std::forward<Args>(args)...);
				};
		}

		Ret InvokeSelf(Args&&... args) const
		{
			if (auto instance = pInstance.lock())
			{
				return mProxy(instance.get(), std::forward<Args>(args)...);
			}
			throw std::runtime_error("Instance expired for member function");
		}

		bool IsValid() const
		{
			return !pInstance.expired();
		}

		virtual bool Invoke(void* param) override
		{
			auto* tuplePtr = static_cast<std::tuple<Args&&...>*>(param);

			std::apply([this](auto&&... args)
				{
					if(IsValid())
					{
						InvokeSelf(std::forward<Args>(args)...);
					}
				}, *tuplePtr);

			return true;
		}
	};

	template<typename Callable>
	// template<typename Ret, typename... Args>
	class EventCallableContainer : public IEventContainer
	{
	private:
		// using Callable = std::function<Ret(Args...)>;
		Callable mProxy;

		using Traits = FunctionTraits<Callable>;
		using ReturnType = typename Traits::ReturnType;
		using ArgTypeOrigin = typename Traits::ArgTypeOrigin;

	public:
	
		EventCallableContainer(Callable func)
		{
			mProxy = std::move(func);
		}

		~EventCallableContainer(){}

		ReturnType InvokeSelf(auto&&... args) const
		{
			if (IsValid())
			{
				return mProxy(args...);
			}
			throw std::runtime_error("Instance expired for member function");
		}

		bool IsValid() const { return mProxy != nullptr; }

		virtual bool Invoke(void* param) override
		{
			auto* tuplePtr = static_cast<ArgTypeOrigin*>(param);

			std::apply([this](auto&&... args)
				{
					if(IsValid())
					{
						InvokeSelf(args...);
					}
				}, *tuplePtr);

			return true;
		}
	};

#pragma endregion

}


template <typename T>
struct HasMemberVariables
{
private:
	template <typename U>
	static auto HasMember(int) -> decltype(
		// 尝试访问成员变量
		std::declval<U>().*(&U::__dummy_member_check),
		std::true_type{}
		);

	template <typename>
	static std::false_type HasMember(...);

public:
	static constexpr bool value = decltype(HasMember<T>(0))::value;
};

export template <typename Derived, typename BaseT>
class Helper : public BaseT
{
public:
	// using Base::Base;

	Helper() = delete;
	Helper(Helper&) = delete;
	Helper(const Helper&) = delete;
	Helper(Helper&&) = delete;
	~Helper() = default;

	Helper& operator=(const Helper&) = delete;
	Helper& operator=(Helper&&) = delete;

	void* operator new(size_t) = delete;
	void operator delete(void*) = delete;

	using BaseT::BaseT;

	using Ptr = std::shared_ptr<Derived>;

	BaseT* Base()
	{
		static_assert(std::is_base_of_v<BaseT, Derived>, "T must inherit from BaseT");
		return static_cast<BaseT*>(this);
	}

	static_assert(!HasMemberVariables<Derived>::value, "Derived must not have member variables");
};
