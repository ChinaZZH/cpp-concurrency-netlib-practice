#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <atomic>
#include <memory>
#include <future>
#include "DBTask.h"
#include "DBConnectionPool.h"
#include "../Common/BlockingConcurrentQueue.h"


namespace MySql
{
    class DBThreadPool
    {
    public:
        DBThreadPool(std::unique_ptr<DBConnectionPool> pool, size_t thread_count = 4);
        ~DBThreadPool();
        
        // 同步执行sql语句
        DBResult ExecuteSync(std::shared_ptr<DBTask> task);

        // 提交一个任务（线程安全，由 Reactor 调用） 
        void SubmitTask(std::shared_ptr<DBTask> task);

        // 停止所有工作线程（等待当前任务完成）
        void Stop();


        // 获取任务队列大小（调试用）
        size_t PendingCount() const;

    private:
        // 工作线程主循环  
        void WorkerLoop();

        // 执行单个任务
        void ExecuteTask(std::shared_ptr<DBTask> task);

        // 执行 SQL 查询（SELECT 等）
        DBResult ExecuteQuery(MYSQL* conn, const std::string& sql);

        // 执行更新（INSERT/UPDATE/DELETE 等）
        DBResult ExecuteUpdate(MYSQL* conn, const std::string& sql);

        // 解析结果集
        DBResult ParseResultSet(MYSQL_RES* res);

    private:
        std::vector<std::thread> workers_;
        moodycamel::BlockingConcurrentQueue<std::shared_ptr<DBTask>> task_data_;
        std::atomic<int> task_count_ = 0;

        std::atomic<bool> stop_ = false;
        std::unique_ptr<DBConnectionPool> pool_;
    };
}

