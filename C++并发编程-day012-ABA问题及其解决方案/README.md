markdown

## Day12： ABA问题及其解决发难

## 一.什么是ABA问题？
ABA问题是无锁编程中的一个经典陷阱。考虑以下场景:

1.线程T1从共享变量中读取数值A。(A可以是指针)。

2.线程T1被操作系统挂起(或发生时间片切换)。如下面的一张截图：
         <img width="840" height="289" alt="111" src="https://github.com/user-attachments/assets/2440cc32-d2cd-41b9-93f6-22f7cc8946eb" />


3.线程T2将共享变量从A修改成B，然后又修改回A(中间可能释放了原有A的内存地址，并重新分配得到了相同地址。)。如下面的两张截图:
        释放了原有的A的内存地址并且修改成B
        <img width="915" height="405" alt="222" src="https://github.com/user-attachments/assets/111bed59-7317-47ab-a4c8-702e60ad2b15" />
        重新分配得到了相同的地址
        <img width="884" height="400" alt="333" src="https://github.com/user-attachments/assets/31938c69-9fa2-48ef-bb9a-1ff392ee8d1f" />

4. 线程T1恢复执行，发现共享变量仍然是A，于是CAS操作成功。但此时A代表的实际数据已经发生了改变(甚至已经被删除)，导致数据结构损坏。如下面的一张截图:

   <img width="910" height="485" alt="444" src="https://github.com/user-attachments/assets/f975188f-4ad2-4ec5-b206-acccaeb01c30" />

危害: 数据丢失，内存错误。

## 解决方案一: 版本号(Tagged Pointer)。

1. 最佳应用场景：无锁栈、对象池、固定大小循环队列

    无锁栈（Lock-Free Stack）：栈的 head 指针频繁被 push / pop 修改，且节点可能被复用（如对象池）。它不需要额外的内存回收机制（节点删除仍需手动），只解决 ABA 判断问题。
                             在栈、队列等单指针修改的场景中实现简单，性能极高。

    内存池管理：固定大小的内存块反复分配释放，地址易重现。

    要求极低延迟、不依赖外部回收机制。

2. 不适合的场景： 节点生命周期不可控、需自动回收内存

3. 游戏服务端典型应用： 对象池、Entity 复用、无锁栈

4. 原理： 将单一的指针扩展为 指针 + 版本号 的组合。每次修改指针时，同时递增版本号。即使指针地址相同，版本号也会不同，从而避免 CAS 误判。

5. 实现方式: 使用 std::atomic<std::pair<Node*, uint64_t>> 或平台相关的双字 CAS（如 __int128）。仅当指针和版本号都匹配时，CAS 才成功。

6.示例(简化)
      struct TaggedPtr {
          Node* ptr;
          uint64_t tag;
      };
      std::atomic<TaggedPtr> head;
      
      void push(Node* node) {
          TaggedPtr old = head.load();
          TaggedPtr new_node = {node, old.tag + 1};
          node->next = old.ptr;
          while (!head.compare_exchange_weak(old, new_node)) {
              node->next = old.ptr;
              new_node = {node, old.tag + 1};
          }
      }

7.优缺点：

      ✅ 原理简单，能彻底解决 ABA（只要 tag 不溢出）。

      ❌ 需要双字 CAS（部分平台不支持），且 tag 有溢出风险（64 位基本安全）。


## 解决方案二: 风险指针（Hazard Pointer）

1. 最佳应用场景：通用无锁队列（MPSC/MPMC）、通用无锁数据结构库。

      无锁队列（尤其是 MPMC、MPSC）：生产者和消费者同时操作，节点动态创建和销毁。

      通用无锁数据结构库（如 libcds），需要完全避免悬空指针。

      多线程环境，节点生命周期不可控（无法预测何时能安全删除）。

2. 不适合的场景： 实现复杂度高、对延迟要求极高

3. 高性能消息队列、指令管道

4. 为什么适合？
   风险指针通过延迟删除机制，保证正在被其他线程访问的节点不会被释放。它不需要全局停顿，与垃圾回收不同，性能较高。
   尤其适合队列这种需要频繁删除和插入的结构，能彻底解决 ABA 和内存安全问题。

5. 游戏服务端例子
    战斗服务器的指令队列：多个生产者（玩家输入）写入，一个消费者（逻辑线程）读取。队列节点动态分配，消费者删除时需确保没有生产者还在引用该节点（风险指针可做到）。
   场景对象的更新队列：类似。

6. 原理
    每个线程在访问共享节点前，将节点地址存入自己的 风险指针 中，表示“我正在使用该节点”。
   准备删除节点时，检查所有线程的风险指针，如果没有任何风险指针指向该节点，则安全删除；否则延迟到以后删除。

6. 工作流程（以 pop 为例）
    thread_local Node* hazard = nullptr;

    Node* pop() {
        while (true) {
            Node* old_head = head.load();
            if (!old_head) return nullptr;
            hazard = old_head;                     // 标记风险
            if (head.load() != old_head) continue; // 二次确认
            Node* next = old_head->next;
            if (head.compare_exchange_strong(old_head, next)) {
                hazard = nullptr;                  // 清除风险
                retire(old_head);                  // 延迟回收
                return old_head;
            }
            hazard = nullptr;
        }
    }

7.优缺点

    ✅ 真正安全，不会出现悬空指针。

    ❌ 实现复杂（需维护全局风险指针列表和回收队列），有一定性能开销。

## 解决方案三: 引用计数（Reference Counting）

1. 最佳应用场景： 单向无锁结构（栈/队列）、已使用 shared_ptr 的项目

    无锁栈、队列（单向链表结构）：节点只有前向指针，不会产生循环引用。

    已有 std::shared_ptr 使用习惯，且能接受原子操作开销。

    可配合 std::atomic<std::shared_ptr<T>>（C++20 前性能较差，但 C++20 有所改进）。

2. 不适合的场景： 双向链表、环形结构（循环引用）

3. 游戏服务端典型应用： 技能链、资源管理、简单无锁栈

4. 为什么适合？

   引用计数天然防止节点在使用时被销毁，计数器为 0 时自动回收，从而避免 ABA。

   它不需要像风险指针那样维护全局列表，代码相对简单。但在双向链表或带环结构中会因循环引用导致内存泄漏，因此只推荐用于单向结构。

6. 游戏服务端例子
   
    技能系统的 Effect 链：每个 Effect 通过 next 指针连接，Effect 可能在多个线程中被引用（如技能释放线程、伤害计算线程）。

    使用引用计数可安全删除，避免 ABA。资源缓存中的共享对象（如纹理、模型），使用引用计数自动管理生命周期。

6.原理：每个节点记录当前被引用的次数。访问节点前增加计数，访问结束后减少。当计数为 0 时才能删除。这样可以保证正在被访问的节点不会被回收，从而避免 ABA。

7.示例（结合 std::shared_ptr）
    struct Node {
        int data;
        std::shared_ptr<Node> next;
        Node(int val) : data(val) {}
    };
    std::atomic<std::shared_ptr<Node>> head;
    
    void push(int val) {
        auto node = std::make_shared<Node>(val);
        auto old = head.load();
        do {
            node->next = old;
        } while (!head.compare_exchange_weak(old, node));
    }

8.优缺点

    ✅ 避免 ABA，自动内存管理（借助 shared_ptr）。

    ❌ 循环引用可能导致内存泄漏（在单向链表中不存在）；shared_ptr 本身有一定开销。


## 以下用表格来说明
|  方案  |    适用场景    |  实现难度  |  性能  |   内存安全   |     游戏应用例子     |
|--------|----------------|------------|--------|-------------|------------------------------|
|版本号|	无锁栈、对象池、固定大小队列|低|极高|需手动回收|实体对象池、无锁栈|
|风险指针| 通用无锁队列、数据结构库|	高|中|完全安全|命令队列、更新队列|
|引用计数| 单向无锁结构|中|	中|安全（无循环）|技能链、共享资源|
