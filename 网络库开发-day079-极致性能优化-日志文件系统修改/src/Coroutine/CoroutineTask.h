#pragma once

#include <coroutine>
#include <exception>
#include <utility>
#include <future>
#include <optional>
#include <iostream>

template<typename T>
struct CoroutineTask
{
    struct promise_type {
        T value;
        std::exception_ptr exception;
        //std::coroutine_handle<> caller; // 存储调用者句柄

        CoroutineTask get_return_object() {
            std::cout << "CoroutineTask::get_return_object pointer=" << this << std::endl;
            return CoroutineTask{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept { std::cout << "CoroutineTask::initial_suspend pointer=" << this << std::endl; return {}; }
        std::suspend_always final_suspend()   noexcept { 
            std::cout << "CoroutineTask::final_suspend pointer=" << this << std::endl;
            /*
            if(caller)
            {
                 std::cout << "Resuming caller" << std::endl;
                caller.resume();
            }
            */

            return {}; 
        }

        void return_value(T val) noexcept { value = std::move(val); }
        void unhandled_exception() { exception = std::current_exception(); } 
    };

    
    CoroutineTask(std::coroutine_handle<promise_type>&& h) :coro(std::move(h))
    { }

    ~CoroutineTask()
    {
        if(coro) 
        {
            coro.destroy();
        }
    }

    CoroutineTask& operator=(CoroutineTask&& other)
    {
        if(this != &other)
        {
            if(coro) 
            {
                coro.destroy();
            }

            coro = std::exchange(other.coro, {});
        }

        return (*this);
    }


    CoroutineTask(CoroutineTask&& other)
    :coro(std::exchange(other.coro, {}))
    {

    }

    CoroutineTask& operator=(const CoroutineTask& other) = delete;
    CoroutineTask(const CoroutineTask& other) = delete;

    // 流程函数，是否已经完成
    bool await_ready() const noexcept { 
        std::cout << "CoroutineTask::await_ready done=" << coro.done() << " ponter=" << this << std::endl;
        return coro.done(); 
    }
    
    void await_suspend(std::coroutine_handle<> handler)  noexcept { 
        //coro.promise().caller = caller;
        coro.resume();
        std::cout << "CoroutineTask::await_suspend ponter=" << this << std::endl; 
    }
    
    T await_resume() const {
        std::cout << "CoroutineTask::await_resume ponter=" << this << std::endl;
        if(coro.promise().exception)
        {
            std::rethrow_exception(coro.promise().exception);
        }

        return std::move(coro.promise().value);
    }

    void resume()
    {
        if(!coro.done())
        {
            coro.resume();
        }
    }

private:
    std::coroutine_handle<promise_type> coro;
};