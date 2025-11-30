export module FuncUtils;

import std;
import StrUtils;

export
{

#pragma region FunctionTraits

	template<typename T>
	struct FunctionTraits;

	template<typename Ret, typename... Args>
	struct FunctionTraits<Ret(*)(Args...)>
	{
		using ReturnType = Ret;
		using ArgType = std::tuple<std::decay_t<Args>...>;
		using ArgTypeForaward = std::tuple<Args&&...>;
		
		static constexpr bool IsMemberFunction = false;
	};

	template<typename Ret, typename Class, typename... Args>
	struct FunctionTraits<Ret(Class::*)(Args...)>
	{
		using ReturnType = Ret;
		using ArgType = std::tuple<std::decay_t<Args>...>;
		using ArgTypeForaward = std::tuple<Args&&...>;

		using ClassType = Class;

		static constexpr bool IsMemberFunction = true;

	};

	template<typename Ret, typename... Args>
	struct FunctionTraits<std::function<Ret(Args...)> >
	{
		using ReturnType = Ret;
		using ArgType = std::tuple<std::decay_t<Args>...>;
		using ArgTypeForaward = std::tuple<Args&&...>;

		using FuncSign = std::function<Ret(Args...)>;
		
		static constexpr bool IsMemberFunction = false;
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

	template<typename Func>
	class DynamicEventContainer;


	template<typename Ret, typename... Args, Ret(*Func)(Args...)>
	class EventContainer<Func> : public IEventContainer
	{
		using Traits = FunctionTraits<decltype(Func)>;
		using ArgType = typename Traits::ArgType;
		using ArgTypeForaward = typename Traits::ArgTypeForaward;

		// 内存空间
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
					return Func(args...);
				};
		}

		~EventContainer(){}

		Ret operator()(auto&&... args) const
		{
			return mProxy(args...);
		}

		virtual bool Invoke(void* param) override
		{
			auto* tuplePtr = static_cast<ArgTypeForaward*>(param);

			std::apply([this](auto&&... args)
				{
					this->operator()(args...);
				}, *tuplePtr);

			return true;
		}
	};

	template<typename Ret, typename Class, typename... Args, Ret(Class::* Func)(Args...)>
	class EventContainer<Func> : public IEventContainer
	{
		using Traits = FunctionTraits<decltype(Func)>;
		using ArgType = typename Traits::ArgType;
		using ArgTypeForaward = typename Traits::ArgTypeForaward;
	private:
		// 事件代理
		std::weak_ptr<Class> pInstance;

		// 空间代理
		Class* pInstanceOrigin = nullptr;

		// 内存空间
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
					return (obj->*Func)(args...);
				};
		}

		EventContainer(Class* instance) : EventContainer()
		{
			pInstanceOrigin = instance;
		}
	
		EventContainer(const std::shared_ptr<Class>& instance) : EventContainer()
		{
			pInstance = instance;
		}

		Ret operator()(auto&&... args) const
		{
			if(pInstanceOrigin)
			{
				return mProxy(pInstanceOrigin, args...);
			}

			if (auto instance = pInstance.lock())
			{
				return mProxy(instance.get(), args...);
			}
			
			throw std::runtime_error("Tick EventContainer Error");
		}

		bool IsValid() const
		{
			return !pInstance.expired();
		}

		virtual bool Invoke(void* param) override
		{
			auto* tuplePtr = static_cast<ArgTypeForaward*>(param);

			std::apply([this](auto&&... args)
				{
					if(IsValid())
					{
						(*this)(args...);;
					}
				}, *tuplePtr);

			return true;
		}
	};

	template<typename Func>
	class DynamicEventContainer : public IEventContainer
	{
	private:
		Func mProxy;

		using Traits = FunctionTraits<Func>;
		using Ret = typename Traits::ReturnType;
		using ArgType = typename Traits::ArgType;
		using ArgTypeForaward = typename Traits::ArgTypeForaward;

	public:
	
		DynamicEventContainer(Func* func) : mProxy(std::move(*func))
		{
			constexpr auto typeHash = TupleTypeHash<ArgType>();
			mTypeHash = typeHash;

#if DN_DEBUG_EVENT
			constexpr auto typeSign = TupleTypeStr<ArgType>();
			sTypeSign = typeSign;
#endif
		}

		virtual ~DynamicEventContainer(){}

		Ret operator()(auto&&... args) const
		{
			return mProxy(args...);
		}

		bool IsValid() const { return mProxy != nullptr; }

		virtual bool Invoke(void* param) override
		{
			auto* tuplePtr = static_cast<ArgTypeForaward*>(param);

			std::apply([this](auto&&... args)
				{
					if(IsValid())
					{
						(*this)(args...);
					}
				}, *tuplePtr);

			return true;
		}
	};

#pragma endregion

}

export template <typename Derived, typename Base>
class Helper : public Base
{
public:

	using Base::Base;
	using Ptr = std::shared_ptr<Derived>;
	using CVPtr = const Ptr&;
	
	Helper() = delete;
	Helper(Helper&) = delete;
	Helper(const Helper&) = delete;
	Helper(Helper&&) = delete;
	~Helper() = default;
	
	Helper& operator=(const Helper&) = delete;
	Helper& operator=(Helper&&) = delete;
	
	void* operator new(size_t) = delete;
	void operator delete(void*) = delete;
	
	Base* GetBase()
	{
		return static_cast<Base*>(this);
	}
};
