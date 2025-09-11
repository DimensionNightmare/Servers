export module FuncUtils;

import std;

// 基础模板声明
export template <auto Func>
class FunctionContainer;

// 特化：普通函数和静态函数
export template <typename Ret, typename... Args, Ret(*Func)(Args...)>
class FunctionContainer<Func>
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
export template <typename Ret, typename Class, typename... Args, Ret(Class::* Func)(Args...)>
class FunctionContainer<Func>
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
	std::function<Ret(Class*,Args...)> mProxy;
};

// 特化：const 成员函数
export template <typename Ret, typename Class, typename... Args, Ret(Class::* Func)(Args...) const>
class FunctionContainer<Func>
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
	std::function<Ret(Class*,Args...)> mProxy;
};
