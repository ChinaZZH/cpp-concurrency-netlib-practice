#include "DBConnectionPool.h"
#include <iostream>
#include <chrono>

namespace MySql
{
    DBConnectionPool::~DBConnectionPool()
    {
        CloseAll();
    }

    bool DBConnectionPool::Init(
        const std::string& host, 
        const std::string& user, 
        const std::string& password,
        const std::string& db, 
        int port  /*= 3306*/, 
        int pool_size /*= 5*/)
    {
        host_ = host;
        user_ = user;
        password_ = password;
        db_ = db;
        port_ = port;
        pool_size_ = pool_size;
        stop_ = false;

        // 初始化 MySQL 库（线程安全）
        mysql_library_init(0, nullptr, nullptr);

        // 创建 pool_size 个连接
        for(int i = 0; i < pool_size_; ++i)
        {
            MYSQL* conn = this->CreateConnection();
            if(!conn)
            {
                std::cerr << "[DB] Failed to create connection " << i << std::endl;
                return false;
            }

            idle_connections_.push(conn);
        }

        std::cout << "[DB] Connection pool initialized with " << pool_size_  << " connections to " << host_ << ":" << port_ << "/" << db_ << std::endl;
        return true;
    }

    // 获取一个连接（如果池中没有可用连接，会阻塞等待）
    std::shared_ptr<MYSQL> DBConnectionPool::GetConnection()
    {
        MYSQL* mysql_conn = nullptr;
        {
            std::unique_lock<std::mutex> lk(mutex_);
            cv_.wait(lk, [this](){ return stop_.load() || !idle_connections_.empty(); });

            if(stop_.load())
            {
                throw std::runtime_error("[DB] Connection pool is shutting down");
            }

            mysql_conn = idle_connections_.front();
            idle_connections_.pop();
        }

        if(!mysql_conn)
        {
           throw std::runtime_error("[DB] Connection pool conn is nullptr");
        }

        // 检查连接是否有效
        if(!IsConnectionValid(mysql_conn))
        {
            // 无效连接直接关闭
            mysql_close(mysql_conn);

            // 尝试创建新连接（补充池中的缺失）
            mysql_conn = CreateConnection();
            if(!mysql_conn)
            {
                throw std::runtime_error("[DB] Failed to create a new connection");
            }
        }

        auto deleter = [this](MYSQL* c){
            if(!c)
            {
                return ;
            }

            if(stop_.load())
            {
                mysql_close(c);
                return;
            }

            // 回滚未提交的事务（防止残留事务）
            mysql_rollback(c);

            // 恢复自动提交模式
            mysql_autocommit(c, 1);

            // 检查连接是否仍然有效
            if(!IsConnectionValid(c))
            {
                mysql_close(c);
                c = CreateConnection();
            }

            // 有效：归还到池中
            {
                std::lock_guard<std::mutex> lk(mutex_);
                idle_connections_.push(c);
            }

            cv_.notify_one();
        };

        std::shared_ptr<MYSQL> shared_ptr_con(mysql_conn, deleter);
        return shared_ptr_con;
    }

    // 获取连接池状态
    size_t DBConnectionPool::IdleCount() const
    {
        std::lock_guard<std::mutex> lk(mutex_);
        return idle_connections_.size();
    }

    size_t DBConnectionPool::TotalCount() const
    {
        return pool_size_;
    }

    // 关闭所有连接
    void DBConnectionPool::CloseAll()
    {
        {
            stop_ = true;
            cv_.notify_all();
        }
        
        {
            std::lock_guard<std::mutex> lk(mutex_);
            while(false == idle_connections_.empty())
            {
                MYSQL* conn = idle_connections_.front();
                idle_connections_.pop();
                mysql_close(conn);
            }
        }

        mysql_library_end();
        std::cout << "[DB] Connection pool closed." << std::endl;
    }

    // 创建一个新的mysql*连接
   MYSQL* DBConnectionPool::CreateConnection()
    {
        MYSQL* conn = mysql_init(nullptr);
        if(!conn)
        {
            std::cerr << "[DB] mysql_init() failed" << std::endl;
            return nullptr;
        }

        // 设置连接超时（10秒）
        unsigned int timeout_seconds = 10;
        mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout_seconds);

        // 启用自动重连
        bool reconnect = true;
        mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);

        if(!mysql_real_connect(conn, host_.c_str(), user_.c_str(), password_.c_str(), db_.c_str(), port_, nullptr, 0))
        {
            std::cerr << "[DB] mysql_real_connect() failed: " << mysql_error(conn) << std::endl;
            mysql_close(conn);
            return nullptr;
        }

        // 默认自动提交模式
        mysql_autocommit(conn, 1);
        return conn;
    }

    // 检查连接是否有效（ping）
    bool DBConnectionPool::IsConnectionValid(MYSQL* conn)
    {
        if(!conn)
        {
            return false;
        }

        // 使用 ping 测试连接是否存活
        return mysql_ping(conn) == 0;
    }
}