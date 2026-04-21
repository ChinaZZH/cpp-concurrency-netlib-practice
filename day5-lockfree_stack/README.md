
markdown

# Day5： 无锁栈 (lock_free stack)

## 核心收获
-- 1.知道了如何使用compare_exchange_weak. 当他失败的时候也就是比较两个值并不相当的时候会更新会把最新的原子变量更新到第一个参数进来。
  同时这个weak即使是相同的情况下也会伪失败，所以需要使用while循环尝试知道他真正的成功。

-- 2.使用compare_exchange_weak, 如果当前原子变量的值老是不相等，则有可能导致活锁。就是在很长一段时间内或者说永远都获取不到真正的最新的数值。
  所以失败的时候需要使用std::memory_order_acquire 更新其他线程同步过来的数值，以保证能及时更新数值。

-- 3.std::memory_order_acquire 和 std::memory_order_release 这个有两个功能。就是 保证指令重排的局部顺序和共享变量的同步和更新。
  如果在程序中对于一个共享变量只使用一个，没有成对使用，则只能实现指令重排的局部顺序 不能进行线程的数据同步了。

-- 4. pop的时候，进入while循环其实也改变了head的数值，head是共享变量也需要使用 std::memory_order_release 同步到其他线程。

-- 5. push的时候 可以使用 new_node->next 去替换 head_node. 这样当他cas失败的时候 会先执行 new_node->next = head. 可以简化代码。
   不过可以使用代码中的例子2 这样逻辑也可以比较简单。

-- 6. compare_exchange_weak 这个函数中的cas 的动作包含了 读-改-写， 这样是一体的。并且读的默认使用的是 std::memory_order_acquire,
   写默认使用的是 std::memory_order_release. 所以 push 中使用 compare_exchange_weak 成功的内存序列是 std::memory_order_release，
    就算是这个 读旧值默认的也是 std::memory_order_acquire。

## 代码
-- lock_free_stack.cpp

## 测试
-- 一切正常。
