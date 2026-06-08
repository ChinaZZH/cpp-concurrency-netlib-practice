#pragma once

#include <atomic>
#include <vector>
#include <thread>
#include <memory>
#include <cstddef>

template<typename T>
class MpmcHazardQueue {
private:
    struct Node {
        T data;
        std::atomic<Node*> next;
		Node() : data(T()), next(nullptr) {}  // 默认构造函数
        explicit Node(const T& d) : data(d), next(nullptr) {}
    };

   	static constexpr int MAX_THREADS = 128;
    static inline std::atomic<Node*> hazard_ptrs[MAX_THREADS] = {};
    static inline std::atomic<int> thread_counter{0};
    static inline thread_local int thread_id = -1;
    static inline std::atomic<Node*> retired_list{nullptr};

    static void register_thread() {
        if (thread_id == -1) {
            thread_id = thread_counter.fetch_add(1);
            hazard_ptrs[thread_id].store(nullptr, std::memory_order_relaxed);
        }
    }

    static void set_hazard(Node* node) {
        hazard_ptrs[thread_id].store(node, std::memory_order_release);
    }

    static void clear_hazard() {
        hazard_ptrs[thread_id].store(nullptr, std::memory_order_release);
    }

    static void retire(Node* node) {
        Node* old = retired_list.load(std::memory_order_relaxed);
        do {
            node->next.store(old, std::memory_order_relaxed);
        } while (!retired_list.compare_exchange_weak(old, node,
                     std::memory_order_release, std::memory_order_relaxed));
    }

    static void scan_and_reclaim() {
        Node* list = retired_list.exchange(nullptr, std::memory_order_acquire);
        if (!list) return;
        // 收集所有风险指针
        std::vector<Node*> hazards;
        int total = thread_counter.load(std::memory_order_acquire);
        for (int i = 0; i < total; ++i) {
            Node* p = hazard_ptrs[i].load(std::memory_order_acquire);
            if (p) hazards.push_back(p);
        }
        while (list) {
            Node* next = list->next.load(std::memory_order_relaxed);
            bool safe = true;
            for (Node* h : hazards) {
                if (h == list) { safe = false; break; }
            }
            if (safe) {
                delete list;   // 自动调用 T 的析构函数
            } else {
                retire(list);
            }
            list = next;
        }
    }

    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;

public:
    MpmcHazardQueue() {
        Node* dummy = new Node();
        head_.store(dummy, std::memory_order_relaxed);
        tail_.store(dummy, std::memory_order_relaxed);
    }

    ~MpmcHazardQueue() {
        // 清理所有节点（简单起见，直接遍历删除）
        Node* node = head_.load();
        while (node) {
            Node* next = node->next.load();
            delete node;
            node = next;
        }
    }

    void enqueue(const T& data) {
        register_thread();
        Node* node = new Node(data);
        while (true) {
            Node* old_tail = tail_.load(std::memory_order_acquire);
            Node* next = old_tail->next.load(std::memory_order_acquire);
            if (next != nullptr) {
                tail_.compare_exchange_weak(old_tail, next);
                continue;
            }
            if (old_tail->next.compare_exchange_weak(next, node)) {
                tail_.compare_exchange_weak(old_tail, node);
                return;
            }
        }
    }

    bool dequeue(T& value) {
        register_thread();
        while (true) {
            Node* old_head = head_.load(std::memory_order_acquire);
            Node* tail = tail_.load(std::memory_order_acquire);
            if (old_head == tail) return false;
            Node* next = old_head->next.load(std::memory_order_acquire);
            if (next == nullptr) return false;
            set_hazard(old_head);
            if (head_.load(std::memory_order_acquire) != old_head) {
                clear_hazard();
                continue;
            }
            if (head_.compare_exchange_weak(old_head, next)) {
                value = std::move(next->data);
                clear_hazard();
                retire(old_head);
                scan_and_reclaim();
                return true;
            }
            clear_hazard();
        }
    }
};
