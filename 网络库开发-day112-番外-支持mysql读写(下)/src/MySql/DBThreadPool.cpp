#include "DBThreadPool.h"
#include <iostream>

namespace MySql
{
    DBThreadPool::DBThreadPool(std::unique_ptr<DBConnectionPool> pool, size_t thread_count /*= 4*/)
    : stop_(false)
    , pool_(std::move(pool))
    {
        workers_.reserve(thread_count);
        for(size_t i = 0; i < thread_count; ++i)
        {
            workers_.emplace_back([this](){
                this->WorkerLoop();
            });
        }

        std::cout << "[DBThreadPool] Created " << thread_count << " worker threads." << std::endl;
    }
    
    DBThreadPool::~DBThreadPool()
    {
        this->Stop();
    }
        
    // 提交一个任务（线程安全，由 Reactor 调用） 
    void DBThreadPool::SubmitTask(std::shared_ptr<DBTask> task)
    {
        if(stop_.load())
        {
            throw std::runtime_error("enqueue on stopped DBThreadPool");
        }

        task_data_.enqueue(task);
        task_count_.fetch_add(1);
    }

     // 同步执行sql语句
    DBResult DBThreadPool::ExecuteSync(std::shared_ptr<DBTask> task)
    {
        if(stop_.load())
        {
            throw std::runtime_error("enqueue on stopped DBThreadPool");
        }

        std::promise<DBResult> sync_promise;
        std::future<DBResult> fut = sync_promise.get_future();
        task->callback = [&sync_promise](const DBResult& result){
            sync_promise.set_value(result);
        };

    
       SubmitTask(task);
       auto status = fut.wait_for(std::chrono::milliseconds(500));
       if(std::future_status::timeout == status)
       {
            DBResult result;
            result.success = false;
            result.error_msg = "execute sync time out";
            return result;
       }

       return fut.get();
    }


    // 停止所有工作线程（等待当前任务完成）
    void DBThreadPool::Stop()
    {
        {
            stop_ = true;
        }
        
        for(auto& worker : workers_)
        {
            if(worker.joinable())
            {
                worker.join();
            }
        }
    }

    // 获取任务队列大小（调试用）
    size_t DBThreadPool::PendingCount() const
    {
        return task_count_;
    }


    // 工作线程主循环  
    void DBThreadPool::WorkerLoop()
    {
        while(false == stop_.load())
        {
            // 由于没有设置cv操作，就是事件通知机制则需要设置唤醒时间, 为了减少cpu无效切换将唤醒时间设置为10秒
            // 就是关机的时候要多等几秒
            std::shared_ptr<DBTask> task;
            if(task_data_.wait_dequeue_timed(task, std::chrono::seconds(10)))
            {
                
                if(task)
                {
                    task_count_.fetch_add(-1);
                    ExecuteTask(task);
                }
            }
        }
    }

    // 执行单个任务
    void DBThreadPool::ExecuteTask(std::shared_ptr<DBTask> task)
    {
        if(!task)
        {
            throw std::runtime_error("DBThreadPool::ExecuteTask task nullptr");
        }

        DBResult result;
        try {
            auto conn_ptr = pool_->GetConnection();
            if(!conn_ptr)
            {
                result.success = false;
                result.error_msg = "Failed to get DB connection";
                if(task->callback)
                {
                    task->callback(result);
                }

                return;
            }

            // 判断 SQL 类型：SELECT 使用 mysql_store_result，其他使用 mysql_affected_rows
            std::string lower_sql = task->sql;
            // 简单判断（实际可做更精确的解析）
            bool is_query = false;
            // 去除前导空格后，检查是否以 SELECT/SHOW/DESCRIBE/EXPLAIN 开头
            size_t pos = lower_sql.find_first_not_of(" \t\n\r");
            if(pos != std::string::npos){
                char first = std::tolower(lower_sql[pos]);
                if('s'== first || 'd' == first || 'e' == first)
                {
                    is_query = true;
                }
            }

            if(is_query)
            {
                result = std::move(ExecuteQuery(conn_ptr.get(), task->sql));
            }
            else
            {
                result = std::move(ExecuteUpdate(conn_ptr.get(), task->sql));
            }

        }
        catch(const std::exception& e)
        {
            result.success = false;
            result.error_msg = e.what();
        }

        if(task->callback)
        {
            task->callback(result);
        }
    }

    // 执行 SQL 查询（SELECT 等）
    DBResult DBThreadPool::ExecuteQuery(MYSQL* conn, const std::string& sql)
    {
        // std::cout << "DBThreadPool::ExecuteQuery sql_context:=" << sql.c_str() << std::endl;
        DBResult result;
        
        if(mysql_query(conn, sql.c_str()) != 0)
        {
            result.success = false;
            result.error_msg = mysql_error(conn);
            return result;
        }

        MYSQL_RES* res = mysql_store_result(conn);
        if(!res)
        {
            result.success = false;
            result.error_msg = "mysql_store_result failed: " + std::string(mysql_error(conn));
            return result;
        }

        return std::move(this->ParseResultSet(res));
    }

    // 执行更新（INSERT/UPDATE/DELETE 等）
    DBResult DBThreadPool::ExecuteUpdate(MYSQL* conn, const std::string& sql)
    {
        DBResult result;
       
        if(mysql_query(conn, sql.c_str()) != 0)
        {
            result.success = false;
            result.error_msg = mysql_error(conn);
            return result;
        }

        result.success = true;
        result.affected_rows = mysql_affected_rows(conn);
        result.insert_id = mysql_insert_id(conn);
        return result;
    }

    // 解析结果集
    DBResult DBThreadPool::ParseResultSet(MYSQL_RES* res)
    {
        DBResult result;
        result.success = true;

        // 获取列信息
        int num_fields = mysql_num_fields(res);
        result.columns.reserve(num_fields);

        MYSQL_FIELD* fields = mysql_fetch_fields(res);
        for(int i = 0; i < num_fields; ++i)
        {
            result.columns.emplace_back(fields[i].name);
        }

        // 获取数据行
        MYSQL_ROW row;
        while((row = mysql_fetch_row(res)))
        {
            std::vector<std::string> row_data;
            row_data.reserve(num_fields);
            for(int i = 0; i < num_fields; ++i)
            {
                if(row[i])
                {
                    row_data.emplace_back(row[i]);
                }
                else
                {
                    row_data.emplace_back("NULL");
                }
            }

            result.rows.push_back(row_data);
        }


        mysql_free_result(res);
        return result;
    }
}
