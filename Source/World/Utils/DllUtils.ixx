module;
export module DllUtils;

import std;
import ThirdParty.Platform;

template<typename T>
struct MemberFunctionReturnType;

template<typename Class, typename ReturnType, typename... Args>
struct MemberFunctionReturnType<ReturnType(Class::*)(Args...)> {
	using RetType = ReturnType;
};

template<typename T>
struct DefaultReturnValue {
	static T get() { return T(); }
};

template<>
struct DefaultReturnValue<void> {
	static void get() {}
};

export template <typename Method>
struct MemberFunctionArgs;

export template <typename R, typename Class, typename... Args>
struct MemberFunctionArgs<R(Class::*)(Args...)>
{
	using Arguments = std::tuple<Args...>;
};

std::unordered_map<std::string, void*> DllMapCache;

export template <typename Class, typename Method, typename... Args>
auto TickMainSpaceDll(Class* obj, Method method, const char* classmethod, Args&&... args)
{
	using ArgsTuple = typename MemberFunctionArgs<decltype(method)>::Arguments;
	using RetType = typename MemberFunctionReturnType<Method>::RetType;
	typedef RetType(*MethodSign)(Class*, ArgsTuple);
	
	if (auto it = DllMapCache.find(classmethod);it != DllMapCache.end())
	{
		MethodSign pFuncTyped = reinterpret_cast<MethodSign>(it->second);
		return pFuncTyped(obj, std::forward_as_tuple(std::forward<Args>(args)...));
	}

	if (Platform::FuncHandle pFunc = Platform::GetProcAddress(nullptr, classmethod))
	{
		DllMapCache[classmethod] = pFunc;
		MethodSign pFuncTyped = reinterpret_cast<MethodSign>(pFunc);
		return pFuncTyped(obj, std::forward_as_tuple(std::forward<Args>(args)...));
	}
	
	return DefaultReturnValue<RetType>::get();
}
