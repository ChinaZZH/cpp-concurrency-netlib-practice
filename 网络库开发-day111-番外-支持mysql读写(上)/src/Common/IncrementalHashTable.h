#pragma once
#include <unordered_map>
#include <utility>
#include <functional>
#include <stdexcept>
#include <cstdint>
#include <optional>
#include <iostream>


template<typename Key, typename Value, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>>

// 渐进式哈希表，避免重哈希
class IncrementalHashTable
{
public:
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>;
    using size_type = std::size_t;
    using hasher = Hash;
    using key_equal = KeyEqual;

    using MapType = std::unordered_map<Key, Value, Hash, KeyEqual>;
    
public:
    // ---------- 构造函数 ----------
    explicit IncrementalHashTable(std::size_t inital_buckets = 8, float load_factor = 0.75, std::size_t migrate_element_num = 2)
    : load_threshold_(load_factor)
    , is_migrating_(false)
    , migration_element_num_(migrate_element_num)
    , inital_primary_buckets_(inital_buckets)
    {

        if(load_factor <= 0.0f)
        {
            throw std::invalid_argument("load_factor must be positive");
        }

        if(migrate_element_num < 2)
        {
            throw std::invalid_argument("migrate_element_num must be at least 2");
        }

        
        primary_.max_load_factor(load_threshold_);
        primary_.reserve(inital_buckets);
        

        //std::cout << "IncrementalHashTable::IncrementalHashTable  primary_buckets:= " << primary_.bucket_count();; 
    }


    // ---------- 插入 ----------
    bool Insert(const value_type& value)
    {
        auto find_result = this->Find(value.first);
        if(find_result)
        {
            return false;
        }

        // 判断当前是否进行迁移操作
        uint32_t primary_buckets_before = primary_.bucket_count();
        if(false == is_migrating_ && this->NeedRehash())
        { 
            this->StartMigration();
        }

       

        if(is_migrating_)
        {
            uint32_t secondary_buckets_before = secondary_.bucket_count();
            //std::cout << "is_migratinging " << std::endl; 
            secondary_.insert(value);

            // 在stl层面发生扩容重哈希的时候打印
            uint32_t primary_buckets_after = primary_.bucket_count();
            uint32_t secondary_buckets_after = secondary_.bucket_count();
            if(secondary_buckets_before != secondary_buckets_after)
             {
                std::cout << " Before insert secondary_bucket_count = " << secondary_buckets_before; 
                std::cout << " , After insert secondary_bucket_count = " << secondary_buckets_after;
                std::cout << std::endl;
            }

            // 同时进行主表向副表的迁移
            MigrateElementForNum(migration_element_num_);
        }
        else
        {
            primary_.insert(value);

            // 在stl层面发生扩容重哈希的时候打印
            uint32_t primary_buckets_after = primary_.bucket_count();
            uint32_t secondary_buckets_after = secondary_.bucket_count();
             if(primary_buckets_before != primary_buckets_after)
             {
                std::cout << " , Before insert: primary_bucket_count = " << primary_buckets_before; 
                std::cout << " :: After insert: primary_bucket_count = " << primary_buckets_after; 
                std::cout << std::endl;
            }
        }    
    
    
        return true;
    }

    // ---------- 更新 ----------
    bool Update(const value_type& value)
    {
        auto find_result = this->Find(value.first);
        if(!find_result)
        {
            return false;
        }

        if(is_migrating_)
        {
            auto itr = secondary_.find(value.first);
            if(itr != secondary_.end())
            {
                (itr->second) = value.second;
                return true;
            }
        }


        auto itr = primary_.find(value.first);
        if(itr != primary_.end())
        {
            (itr->second) = value.second;
            return true;
        }

        return false;
    }

    // ---------- 查找 ----------
    std::optional<Value> Find(const key_type& key)
    {
        if(is_migrating_)
        {
            auto itr = secondary_.find(key);
            if(itr != secondary_.end())
            {
                return itr->second;
            }
        }


        auto itr = primary_.find(key);
        if(itr != primary_.end())
        {
            return itr->second;
        }

        return std::nullopt;
    }

    // ---------- 删除 ----------
    size_t Erase(const key_type& key)
    {
        size_t num = primary_.erase(key);
        if(is_migrating_)
        {
            size_t tmp_num = secondary_.erase(key);
            num += tmp_num;
            if(num > 0)
            {
                MigrateElementForNum(migration_element_num_);
            }
        
        }

        return num;
    }

    // ---------- 删除 ----------
    size_t Size() const
    {
        size_t num = primary_.size();
        if(is_migrating_)
        {
            num += (secondary_.size());
        }

        return num;
    }

private:
    // ---------- 判断是否需要迁移 ----------
    bool NeedRehash() const
    {
        // 如果桶的个数已经大于等于2的31次方，则不进行渐进性的迁移和扩容, 直接走原生默认的。
        std::size_t max_bucket_count = static_cast<std::size_t>(1) << 30;
        if(is_migrating_ || primary_.empty() || primary_.bucket_count() >= max_bucket_count)
        {
            return false;
        }

        // 计算负载因子
        float fRactor = static_cast<float>(primary_.size() + 1) / static_cast<float>(primary_.bucket_count()); 
        return fRactor >= primary_.max_load_factor();
    }


    // ---------- 启动迁移 ----------
    void StartMigration()
    {
        if(is_migrating_)
        {
            return;
        }

        //std::cout << "StartMigration" << std::endl;
        is_migrating_ = true; 

        // 为了防止容器在迁移过程中 备用容器又进行迁移 新的容器进行两倍扩容
        std::size_t new_bucket_count = primary_.bucket_count() * 2;
        inital_primary_buckets_ = new_bucket_count;

        secondary_.max_load_factor(load_threshold_);
        secondary_.reserve(new_bucket_count);
    }
    

    // ---------- 迁移指定数量的桶 ----------
    void MigrateElementForNum(uint32_t num)
    {
        if(false == is_migrating_)
        {
            return;
        }

        for(size_t i = 0; i < num; ++i)
        {
            if(primary_.empty())
            {
                break;
            }

            // 进行迁移
            auto itr = primary_.begin();
            key_type key = (itr->first);
            const mapped_type& value = (itr->second);

            secondary_.insert(std::pair(key, value));
            primary_.erase(itr);
        }

        
        if(primary_.empty())
        {
            FinishMigration();
        }
    }

    // ---------- 完成迁移 ----------
    void FinishMigration()
    {
        // 没有在迁移状态中 或者 迁移状态中但是元素并没有迁移完全，则返回错误
        if(false == is_migrating_ || !primary_.empty())
        {
            return;
        }

        //std::cout << "FinishMigration primary_bucket_count:=" << primary_.bucket_count() << " seconary_bucket_count:=" << secondary_.bucket_count() << std::endl;
        is_migrating_ = false;
        primary_.swap(secondary_);

        MapType empty_map;
        secondary_.swap(empty_map);
    }

private:
    MapType primary_;

    MapType secondary_;
    
    float load_threshold_;
    
    bool is_migrating_ = false;
    
    uint32_t migration_element_num_;

    uint32_t inital_primary_buckets_;
};