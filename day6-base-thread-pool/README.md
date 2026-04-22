markdown

## Day6： 基础的线程池 (BaseThreadPool)
## 核心收获
-- 1.关于AddTask的讨论，通过使用 std::bind 将用户函数和可变参数args绑定会变成一个无参数的可调用函数。 auto bound_func = std::bind(std::forward<F>(f), std::forward<Args>(args)...)。

-- 2.关于AddTask的讨论，通过std::invoke_result_t<F, Args...> 让编译器自动生成函数返回参数的类型。using return_type = std::invoke_result_t<F, Args...>。

-- 3.关于AddTask的讨论，用绑定后的函数构造一个packaged_task ，他返回return_type。 同时 

-- 4. 关于AddTask的讨论，std::packaged_task<R()>::operator() 返回 void。所以std::packaged_task和std::bind 可以用 std::function<void()>进行统一包装。具有泛型的作用。

-- 5. 关于AddTask的讨论，std::forward可以进行完美转发，但是必须与模板参数 T 一起使用。 例如std::forward(arg) 是错误的，必须写成 std::forward<decltype(arg)>(arg) 或 std::forward<T>(arg)

-- 6. 关于AddTask的讨论，packaged_task 只能移动不能构造。

-- 7. 其余内容观看线程池代码。

## 代码
-- ThreadPool.cpp

## 测试
-- 一切正常。
