#pragma once

#include <queue>
#include <vector>
#include <memory>
#include <cstddef>
#include <unordered_set>
#include <functional>

template<typename T>
class ObjectPool
{
public:
    explicit ObjectPool(size_t pre_alloc_count = 0)
    {
        free_list_.reserve(pre_alloc_count);
        for(int i = 0; i < pre_alloc_count; ++i)
        {
           T* ptr = static_cast<T*>(::operator new(sizeof(T)));
           free_list_.push_back(ptr);
           allocated_.insert(ptr);
        }
    }


    ~ObjectPool()
    {
        for(auto& ptr : allocated_)
        {
            auto itr = std::find(free_list_.begin(); free_list_.end(), ptr);
            if(itr == free_list_.end())
            {
                ptr->~T();
            }

            ::operator delete(ptr);
        }
    }

    // 分配对象(从空闲队列取内存， 然后构造)
    template<typename... Args>
    T* Allocate(Args&&... args)
    {
        T* ptr = nullptr;
        if(!free_list_.empty())
        {
            ptr = free_list_.back();
            free_list_.pop_back();
        }else{
             ptr = static_cast<T*>(::operator new(sizeof(T)));
             allocated_.insert(ptr);
        }

        ::new (ptr) T(std::forward<Args>(args)...);
        return ptr; 
    }


    // 归还对象
    bool Deallocate(T* ptr)
    {
        if(!ptr)
        {
            return false;
        }

        // 没有分配过这一块内存
        auto itr = allocated_.find(ptr);
        if(itr == allocated_.end())
        {
            return false;
        }

        // 已经在free_list里面了。
        auto itr = std::find(free_list_.begin(); free_list_.end(), ptr);
        if(itr != free_list_.end())
        {
            return false;
        }

        ptr->~T();
        free_list_.push_back(ptr);
        return true;
    }

    // 返回shared_ptr，自动管理归还
    template<typename... Args>
    std::shared_ptr<T> GetSharedObj(Args&&... args)
    {
        T* ptr = Allocate(std::forward<Args>(args)...);
        return std::shared_ptr<T>(ptr, [this](T* p){
            Deallocate(p);
        });
    }

    // 返回unique_ptr，自动管理归还
    template<typename... Args>
    std::unique_ptr<T, std::function<void(T*)>> GetUniqueObj(Args&&... args)
    {
        T* ptr = Allocate(std::forward<Args>(args)...);
        return std::unique_ptr<T, std::function<void(T*)>>(ptr, [this](T* p){
            Deallocate(p);
        });
    }


    // 禁止拷贝构造和赋值运算符重载
    ObjectPool(const ObjectPool& pool) = delete;
    
    ObjectPool& operator=(const ObjectPool& pool) = delete;

private:
    std::vector<T*>            free_list_;     // 空闲内存空间(已析构，可以取出直接进行构造使用)
    std::unordered_set<T*>     allocated_;     // 所有分配过的内存
};