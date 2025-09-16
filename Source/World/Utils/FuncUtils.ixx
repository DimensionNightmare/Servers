export module FuncUtils;

import std;

export 
{
	// 基础模板声明
	template <auto Func>
	struct FunctionContainer;

	// 特化：普通函数和静态函数
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

		// 调用函数
		Ret operator()(Args... args) const
		{
			return mProxy(std::forward<Args>(args)...);
		}

		std::function<Ret(Args...)> mProxy;
	};

	// 特化：非 const 成员函数
	template <typename Ret, typename Class, typename... Args, Ret(Class::* Func)(Args...)>
	struct FunctionContainer<Func>
	{
	public:
		FunctionContainer(Class* instance)
		{
			SetInstance(instance);
		}

		// 调用函数
		Ret operator()(Args... args) const
		{
			if (!pInstance)
			{
				throw std::runtime_error("Instance not set for member function");
			}
			return mProxy(pInstance, std::forward<Args>(args)...);
		}

	private:
		// 设置对象实例
		void SetInstance(Class* instance)
		{
			pInstance = instance;
			mProxy = [](Class* obj, Args... args) -> Ret
				{
					return (obj->*Func)(std::forward<Args>(args)...);
				};
		}

	private:
		Class* pInstance = nullptr;
		std::function<Ret(Class*, Args...)> mProxy;
	};

	// 特化：const 成员函数
	template <typename Ret, typename Class, typename... Args, Ret(Class::* Func)(Args...) const>
	struct FunctionContainer<Func>
	{
	public:
		FunctionContainer(Class* instance)
		{
			SetInstance(instance);
		}

		// 调用函数
		Ret operator()(Args... args) const
		{
			if (!pInstance)
			{
				throw std::runtime_error("Instance not set for member function");
			}
			return mProxy(pInstance, std::forward<Args>(args)...);
		}

	private:
		// 设置对象实例
		void SetInstance(Class* instance)
		{
			pInstance = instance;
			mProxy = [](Class* obj, Args... args) -> Ret
				{
					return (obj->*Func)(std::forward<Args>(args)...);
				};
		}

	private:
		const Class* pInstance = nullptr;
		std::function<Ret(Class*, Args...)> mProxy;
	};

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

	using Ptr = std::shared_ptr<Derived>;
	using CVPtr = const Ptr&;

	BaseT* Base()
	{
		static_assert(std::is_base_of_v<BaseT, Derived>, "T must inherit from BaseT");
		return static_cast<BaseT*>(this);
	}

	static_assert(!HasMemberVariables<Derived>::value, "Derived must not have member variables");
};
