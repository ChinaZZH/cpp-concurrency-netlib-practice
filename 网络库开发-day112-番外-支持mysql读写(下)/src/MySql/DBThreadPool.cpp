#include "DBThreadPool.h"
#include <iostream>
#include <cstring>

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


    /*
    DBResult DBThreadPool::ExecuteSyncPrepared(uint32_t player_id, const std::string& sql, const std::vector<std::string>& params)
    {
        if(stop_.load())
        {
            throw std::runtime_error("enqueue on stopped DBThreadPool");
        }

        std::shared_ptr<DBTask> task = std::make_shared<DBTask>();
        task->conn_id = player_id;
        task->sql = sql;
        task->params = params;
        return std::move(ExecuteSync(task));
    }
    */


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

            // 根据是否有参数决定执行路径
            //if(task->params.empty())
            {
                // 路径1：普通 SQL（原有逻辑）
                if(is_query)
                {
                    result = std::move(ExecuteQuery(conn_ptr.get(), task->sql));
                }
                else
                {
                    result = std::move(ExecuteUpdate(conn_ptr.get(), task->sql));
                }
            }
            /*
            else
            {
                // 路径2：预处理语句（新增逻辑）
                if (is_query) 
                {
                    result = std::move(ExecutePreparedQuery(conn_ptr.get(), task->sql, task->params));
                } 
                else {
                    result = std::move(ExecutePreparedUpdate(conn_ptr.get(), task->sql, task->params));
                }
            }
            */
           
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

    /*
    DBResult DBThreadPool::ExecutePreparedQuery(MYSQL* conn, const std::string& sql, const std::vector<std::string>& params)
    {
        // 1. 初始化预处理句柄
        DBResult result;
        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (!stmt) 
        {
            result.success = false;
            result.error_msg = "mysql_stmt_init failed";
            return result;
        }

        // 2. 准备 SQL
        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length()) != 0) 
        {
            result.success = false;
            result.error_msg = "mysql_stmt_prepare failed: " + std::string(mysql_stmt_error(stmt));
            mysql_stmt_close(stmt);
            return result;
        }


        // 3. 获取参数数量，校验一致性
        unsigned int param_count = mysql_stmt_param_count(stmt);
        if (param_count != params.size()) 
        {
            result.success = false;
            result.error_msg = "Parameter count mismatch: expected " + std::to_string(param_count) +
                           ", got " + std::to_string(params.size());
            mysql_stmt_close(stmt);
            return result;
        }


        // 4. 构建 MYSQL_BIND 数组
        if(param_count > 0)
        {
            std::vector<MYSQL_BIND> bind(params.size());
            std::vector<std::vector<char>> buffers(params.size());
            std::vector<unsigned long> lengths(params.size());

            for (size_t i = 0; i < params.size(); ++i) 
            {
                buffers[i].resize(params[i].size() + 1);
                memcpy(buffers[i].data(), params[i].c_str(), params[i].size());
                buffers[i][params[i].size()] = '\0';

                lengths[i] = static_cast<unsigned long>(params[i].size());

                bind[i].buffer_type = MYSQL_TYPE_STRING;
                bind[i].buffer = buffers[i].data();
                bind[i].buffer_length = buffers[i].size();
                bind[i].length = &lengths[i];
                bind[i].is_null = nullptr;
            }

            if (mysql_stmt_bind_param(stmt, bind.data()) != 0) 
            {
                result.success = false;
                result.error_msg = "mysql_stmt_bind_param failed: " + std::string(mysql_stmt_error(stmt));
                mysql_stmt_close(stmt);
                return result;
            }
        }
        
    
        // 5. 执行
        if (mysql_stmt_execute(stmt) != 0) 
        {
            result.success = false;
            result.error_msg = "mysql_stmt_execute failed: " + std::string(mysql_stmt_error(stmt));
            mysql_stmt_close(stmt);
            return result;
        }

        // 6. 获取元数据（列信息）
        MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
        if (!meta) {
            // 没有结果集（如 INSERT/DELETE）
            result.success = true;
            result.affected_rows = mysql_stmt_affected_rows(stmt);
            result.insert_id = mysql_stmt_insert_id(stmt);
            mysql_stmt_close(stmt);
            return result;
        }


         // 7. 获取列数
        int num_fields = mysql_num_fields(meta);
        MYSQL_FIELD* fields = mysql_fetch_fields(meta);
        for (int i = 0; i < num_fields; ++i) {
            result.columns.push_back(fields[i].name);
        }


         // 8. 存储结果并解析
        if (mysql_stmt_store_result(stmt) != 0) 
        {
            result.success = false;
            result.error_msg = "mysql_stmt_store_result failed: " + std::string(mysql_stmt_error(stmt));
            mysql_free_result(meta);
            mysql_stmt_close(stmt);
            return result;
        }


         // 9. 绑定结果列（动态分配缓冲区）
        std::vector<unsigned long> result_lengths(num_fields);
        std::vector<unsigned char> result_is_null(num_fields, 0);
        std::vector<std::vector<char>> result_buffers(num_fields);

        // 先绑定一个占位绑定（仅用于获取长度）
        std::vector<MYSQL_BIND> result_bind(num_fields);

        for (int i = 0; i < num_fields; ++i) {
            // 暂时分配 1 字节，只是为了能绑定
            result_buffers[i].resize(1);
            result_bind[i].buffer_type = MYSQL_TYPE_STRING;
            result_bind[i].buffer = result_buffers[i].data();
            result_bind[i].buffer_length = 0;  // 关键：设为 0，让 MySQL 只返回长度而不存储数据
            result_bind[i].length = &result_lengths[i];
            result_bind[i].is_null = reinterpret_cast<bool*>(&result_is_null[i]);
        }

        if (mysql_stmt_bind_result(stmt, result_bind.data()) != 0) {
            result.success = false;
            result.error_msg = "mysql_stmt_bind_result failed: " + std::string(mysql_stmt_error(stmt));
            mysql_free_result(meta);
            mysql_stmt_close(stmt);
            return result;
        }

        // 10. 逐行获取数据
        while (true) 
        {
            int status = mysql_stmt_fetch(stmt);
            if (status == 1) {
                // 错误
                result.success = false;
                result.error_msg = "mysql_stmt_fetch error: " + std::string(mysql_stmt_error(stmt));
                mysql_free_result(meta);
                mysql_stmt_close(stmt);
                return result;
            } else if (status == MYSQL_NO_DATA) {
                break;  // 所有行已获取完毕
            }

            std::vector<std::string> row_data;
            for (int i = 0; i < num_fields; ++i) 
            {
                if (result_is_null[i]) 
                {
                    row_data.push_back("NULL");
                    continue;
                } 
                

                // 使用 mysql_stmt_fetch_column 获取实际数据
                // 为当前列分配足够大的缓冲区
                std::vector<char> buffer(result_lengths[i] + 1);
                MYSQL_BIND single_bind;
                memset(&single_bind, 0, sizeof(single_bind));
                single_bind.buffer_type = MYSQL_TYPE_STRING;
                single_bind.buffer = buffer.data();
                single_bind.buffer_length = buffer.size();
                single_bind.length = &result_lengths[i];

                if(mysql_stmt_fetch_column(stmt, &single_bind, i, 0) != 0) 
                {
                    // 错误处理
                    row_data.push_back("[ERROR]");
                    continue;
                }

                row_data.push_back(std::string(buffer.data(), result_lengths[i]));
            }


            result.rows.push_back(row_data);
        }


        // 11. 释放资源
        mysql_free_result(meta);
        mysql_stmt_close(stmt);
        result.success = true;
        return result;
    }
    */


    /*
    DBResult DBThreadPool::ExecutePreparedUpdate(MYSQL* conn, const std::string& sql, const std::vector<std::string>& params)
    {
        // 1. 初始化预处理句柄
        DBResult result;
        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (!stmt) 
        {
            result.success = false;
            result.error_msg = "mysql_stmt_init failed";
            return result;
        }


        // 2. 准备 SQL
        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length()) != 0) 
        {
            result.success = false;
            result.error_msg = "mysql_stmt_prepare failed: " + std::string(mysql_stmt_error(stmt));
            mysql_stmt_close(stmt);
            return result;
        }

        // 3. 校验参数数量
        unsigned int param_count = mysql_stmt_param_count(stmt);
        if (param_count != params.size()) 
        {
            result.success = false;
            result.error_msg = "Parameter count mismatch";
            mysql_stmt_close(stmt);
            return result;
        }

        // 4. 构建 MYSQL_BIND 数组
        std::vector<MYSQL_BIND> bind(params.size());
        std::vector<std::vector<char>> buffers(params.size());
        std::vector<unsigned long> lengths(params.size());

        for (size_t i = 0; i < params.size(); ++i) 
        {
            buffers[i].resize(params[i].size() + 1);
            memcpy(buffers[i].data(), params[i].c_str(), params[i].size());
            buffers[i][params[i].size()] = '\0';
            lengths[i] = static_cast<unsigned long>(params[i].size());

            bind[i].buffer_type = MYSQL_TYPE_STRING;
            bind[i].buffer = buffers[i].data();
            bind[i].buffer_length = buffers[i].size();
            bind[i].length = &lengths[i];
            bind[i].is_null = nullptr;
        }


         // 5. 绑定参数
        if (mysql_stmt_bind_param(stmt, bind.data()) != 0) 
        {
            result.success = false;
            result.error_msg = "mysql_stmt_bind_param failed: " + std::string(mysql_stmt_error(stmt));
            mysql_stmt_close(stmt);
            return result;
        }

        // 6. 执行
        if (mysql_stmt_execute(stmt) != 0) 
        {
            result.success = false;
            result.error_msg = "mysql_stmt_execute failed: " + std::string(mysql_stmt_error(stmt));
            mysql_stmt_close(stmt);
            return result;
        }

        // 7. 获取结果
        result.success = true;
        result.affected_rows = mysql_stmt_affected_rows(stmt);
        result.insert_id = mysql_stmt_insert_id(stmt);

        mysql_stmt_close(stmt);
        return result;
    }
    */
}
