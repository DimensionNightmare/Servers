#include <iostream>
#include <coroutine>
#include <map>
#include <unordered_map>

// 定义一个协程函数
struct MyCoroutine {
    struct promise_type {
        MyCoroutine get_return_object() {
            return MyCoroutine(std::coroutine_handle<promise_type>::from_promise(*this));
        }
        std::suspend_never initial_suspend() {
            return {};
        }
        std::suspend_always final_suspend() noexcept {
            return {};
        }
        void return_void() {}
        void unhandled_exception() {}
    };

    void resume() {
        coroutine.resume();
    }

    std::coroutine_handle<promise_type> coroutine;

	MyCoroutine() : coroutine(nullptr) {}

    MyCoroutine(std::coroutine_handle<promise_type> coro) : coroutine(coro) {}
};

// 使用 map 存储协程句柄
std::unordered_map<int, MyCoroutine> coroutineMap;

MyCoroutine MyCoroutineFunction() {
    co_await std::suspend_always{};
    std::cout << "Inside coroutine" << std::endl;
    co_return;
}


// 函数1：保存协程到 map 中
void SaveCoroutineToMap(int id) {
	auto func = []()-> MyCoroutine
	{
		co_await std::suspend_always{};
		std::cout << "Inside coroutine" << std::endl;
		co_return;
	};
    MyCoroutine coroutine = func();
    coroutineMap[id] = coroutine;
}

// 函数2：从 map 中恢复并执行协程
void ResumeCoroutineFromMap(int id) {
    auto it = coroutineMap.find(id);
    if (it != coroutineMap.end()) {
        MyCoroutine& coroutine = it->second;
        coroutine.resume();
    }
}


int main() {
    // 保存协程到 map
    SaveCoroutineToMap(1);

    // 在另一个函数中从 map 中恢复并执行协程
    ResumeCoroutineFromMap(1);

    return 0;
}
