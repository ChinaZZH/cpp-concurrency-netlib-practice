#pragma once

#include <coroutine>
#include <exception>
#include <utility>
#include <future>
#include <optional>
#include <functional>
#include <iostream>


template<typename T>
struct AwaitableCallback
{
    using Callback = std::function<void(T)>;
    using ErrorCallback = std::function<void(std::exception_ptr)>;
    
    std::function<void(Callback, ErrorCallback)> async_operation;

    struct State {
        T result;
        std::exception_ptr exception;
        std::atomic<bool> resumed{false};
    };

    std::shared_ptr<State> state = std::make_shared<State>();

    // 始终挂起，让 await_suspend 注册回调
    bool await_ready() const noexcept {
        std::cout << "AwaitableCallback::await_ready false pointer=" << this << std::endl;
        return false;
    }

    template<typename Promise>
    void await_suspend(std::coroutine_handle<Promise> handle)
    {
        std::cout << "AwaitableCallback::await_suspend pointer=" << this << std::endl;

        auto state_ptr = state;

        // 启动异步操作， 并在完成时候恢复协程
        auto func = [state_ptr, handle](T value){
            if(state_ptr->resumed.exchange(true)){
                std::cout << "Callback ignored (already resumed)" << std::endl;
                return;
            }

            std::cout << "Success callback: setting result" << std::endl;
            state_ptr->result = std::move(value);

            std::cout << "callback handle to resume" << std::endl;
            handle.resume();
        };

        auto errorCallback = [state_ptr, handle](std::exception_ptr e){
            if (state_ptr->resumed.exchange(true)) {
                std::cout << "Error callback ignored (already resumed)" << std::endl;
                return;
            }

            std::cout << "Error callback: setting exception" << std::endl;
            state_ptr->exception = e;

            std::cout << "callback handle to resume" << std::endl;
            handle.resume();
        };

        async_operation(func, errorCallback);
       
    }

    T await_resume() {
        assert(state->resumed);
        std::cout << "AwaitableCallback::await_resume pointer=" << this << std::endl;
        if(state->exception)
            std::rethrow_exception(state->exception);

        return std::move(state->result);
    }
};