#pragma once
#include <mysql/mysql.h>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string>
#include <stdexcept>

namespace MySql
{
    class DBConnectionPool
    {
    public:
        DBConnectionPool() = default;
        ~DBConnectionPool();

        // 禁止拷贝
        DBConnectionPool(const DBConnectionPool& pool) = delete;
        DBConnectionPool& operator=(const DBConnectionPool& pool) = delete;

        // 初始化连接
        bool Init(const std::string& host, const std::string& user, const std::string& password,
                const std::string& db, int port = 3306, int pool_size = 5);

        // 获取一个连接（如果池中没有可用连接，会阻塞等待）
        std::shared_ptr<MYSQL> GetConnection();

        // 获取连接池状态
        size_t IdleCount() const;

        size_t TotalCount() const;

        // 关闭所有连接
        void CloseAll();

    private:
        // 创建一个新的mysql*连接
        MYSQL* CreateConnection();

        // 检查连接是否有效（ping）
        bool IsConnectionValid(MYSQL* conn);


    private:
        std::queue<MYSQL*> idle_connections_;
        mutable std::mutex mutex_;
        std::condition_variable cv_;
        std::atomic<bool> stop_{false};

        std::string host_;
        std::string user_;
        std::string password_;
        std::string db_;
        int port_ = 3306;
        int pool_size_ = 5;
    };
}
