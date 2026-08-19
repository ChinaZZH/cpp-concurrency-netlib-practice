#include "Test_DB_Mysql.h"
#include <iostream>
#include <mysql/mysql.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <memory>
#include "../../MySql/DBConnectionPool.h"
#include "../../MySql/DBThreadPool.h"

int Test_DB_Mysql::Test_Connection_Pool()
{
    MySql::DBConnectionPool pool;
    if (!pool.Init("127.0.0.1", "root", "zzh@890918", "game_server", 3306, 5)) {
        return 1;
    }

     try {
        auto conn = pool.GetConnection();

        if (mysql_query(conn.get(), "SELECT 1")) {
            std::cerr << "Query failed: " << mysql_error(conn.get()) << std::endl;
            return 1;
        }

        MYSQL_RES* res = mysql_store_result(conn.get());
        if (res) {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row) {
                std::cout << "Result: " << row[0] << std::endl;
            }
            mysql_free_result(res);
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    // conn 离开作用域自动归还
    return 0;
}


void Test_DB_Mysql::PrintResult(const MySql::DBResult& result)
{
    if(!result.success) {
        std::cerr << "  [ERROR] " << result.error_msg << std::endl;
        return;
    }

    std::cout << "  Success! ";

    // 执行 update/insert/delete
    if (result.affected_rows > 0) 
    {
        std::cout << "Affected rows: " << result.affected_rows; 
        std::cout << ", Insert ID: " << result.insert_id << std::endl;
        return;
    }

    // 执行select，返回的是空内容
    if (result.rows.empty()) {
        std::cout << "No data returned." << std::endl;
        return;
    }

    // 打印select出来的内容集合
    std::cout << "Rows: " << result.rows.size(); 
    std::cout << ", Columns: " << result.columns.size() << std::endl;

    // 打印列名
    std::cout << "  Columns: ";
    for (const auto& col : result.columns) {
        std::cout << col << " ";
    }
    std::cout << std::endl;

    // 打印前 3 行数据
    size_t row_count = std::min(result.rows.size(), size_t(5));
    for (size_t i = 0; i < row_count; ++i) {
        std::cout << "  Row " << i << ": ";
        for (const auto& field : result.rows[i]) {
            std::cout << field << " ";
        }

        std::cout << std::endl;
    }

    if (result.rows.size() > 5) {
        std::cout << "  ... and " << (result.rows.size() - 5) << " more rows" << std::endl;
    }
}



int Test_DB_Mysql::Test_DB_Task_Pool()
{
    std::cout << "=== DB Thread Pool Test ===" << std::endl;
    // ----------------------------------------------------------------
    // 1. 初始化连接池
    // ----------------------------------------------------------------
    std::cout << "\n[1] Initializing connection pool..." << std::endl;
    std::unique_ptr<MySql::DBConnectionPool> pool;
    {
        pool = std::make_unique<MySql::DBConnectionPool>();
        if (!pool->Init("127.0.0.1", "root", "zzh@890918", "game_server", 3306, 5)) {
            std::cerr << "Failed to initialize connection pool" << std::endl;
            return 1;
        }

        std::cout << "  Idle connections: " << pool->IdleCount() << std::endl;
    } 
   

    // ----------------------------------------------------------------
    // 2. 初始化 DB 线程池（3 个工作线程）
    // ----------------------------------------------------------------
    std::cout << "\n[2] Initializing DB thread pool..." << std::endl;
    MySql::DBThreadPool thread_pool(std::move(pool), 3);
    std::cout << "  Thread pool ready." << std::endl;

    std::atomic<int> tasks_completed{0};
    std::atomic<int> tasks_failed{0};

    // ----------------------------------------------------------------
    // 3. 创建测试表
    // ----------------------------------------------------------------
    std::cout << "\n[3] Creating test table..." << std::endl;
    

     auto create_table_cb = [&](const MySql::DBResult& result) {
        tasks_completed++;
        if (result.success) {
            std::cout << "  Table created successfully." << std::endl;
        } else {
            tasks_failed++;
            std::cerr << "  Table creation failed: " << result.error_msg << std::endl;
        }
    };

    std::shared_ptr<MySql::DBTask> create_task = std::make_shared<MySql::DBTask>();
    create_task->sql = R"(
        CREATE TABLE IF NOT EXISTS test_users (
            id INT AUTO_INCREMENT PRIMARY KEY,
            name VARCHAR(50) NOT NULL,
            age INT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";
    create_task->callback = create_table_cb;
    thread_pool.SubmitTask(create_task);

    // 等待表创建完成
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // ----------------------------------------------------------------
    // 4. 插入测试数据
    // ----------------------------------------------------------------
    std::cout << "\n[4] Inserting test data..." << std::endl;

    auto insert_cb = [&](const MySql::DBResult& result) {
        tasks_completed++;
        if (result.success) {
            std::cout << "  Inserted " << result.affected_rows << " row(s), last insert ID: " 
                      << result.insert_id << std::endl;
        } else {
            tasks_failed++;
            std::cerr << "  Insert failed: " << result.error_msg << std::endl;
        }
    };

    std::shared_ptr<MySql::DBTask> insert_task1 = std::make_shared<MySql::DBTask>();
    insert_task1->sql = "INSERT INTO test_users (name, age) VALUES ('Alice', 25)";
    insert_task1->callback = insert_cb;
    thread_pool.SubmitTask(insert_task1);

    std::shared_ptr<MySql::DBTask> insert_task2 = std::make_shared<MySql::DBTask>();
    insert_task2->sql = "INSERT INTO test_users (name, age) VALUES ('Bob', 30), ('Charlie', 35), ('Diana', 28)";
    insert_task2->callback = insert_cb;
    thread_pool.SubmitTask(insert_task2);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    

    // ----------------------------------------------------------------
    // 5. 查询测试
    // ----------------------------------------------------------------
    std::cout << "\n[5] Querying data..." << std::endl;
    auto query_cb = [&](const MySql::DBResult& result) {
        tasks_completed++;
        if (result.success) {
            std::cout << "  Query successful." << std::endl;
            PrintResult(result);
        } else {
            tasks_failed++;
            std::cerr << "  Query failed: " << result.error_msg << std::endl;
        }
    };

    std::shared_ptr<MySql::DBTask> query_task = std::make_shared<MySql::DBTask>();
    query_task->sql = "SELECT id, name, age FROM test_users ORDER BY id";
    query_task->callback = query_cb;
    thread_pool.SubmitTask(query_task);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

     // ----------------------------------------------------------------
    // 6. 更新测试
    // ----------------------------------------------------------------
    std::cout << "\n[6] Updating data..." << std::endl;

    auto update_cb = [&](const MySql::DBResult& result) {
        tasks_completed++;
        if (result.success) {
            std::cout << "  Updated " << result.affected_rows << " row(s)." << std::endl;
        } else {
            tasks_failed++;
            std::cerr << "  Update failed: " << result.error_msg << std::endl;
        }
    };

    std::shared_ptr<MySql::DBTask> update_task = std::make_shared<MySql::DBTask>();
    update_task->sql = "UPDATE test_users SET age = age + 1 WHERE name = 'Alice'";
    update_task->callback = update_cb;
    thread_pool.SubmitTask(update_task);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

     // ----------------------------------------------------------------
    // 7. 验证更新结果（再查询一次）
    // ----------------------------------------------------------------
    std::cout << "\n[7] Verifying update..." << std::endl;

    auto verify_cb = [&](const MySql::DBResult& result) {
        tasks_completed++;
        if (result.success) {
            std::cout << "  Verify query successful." << std::endl;
            PrintResult(result);
        } else {
            tasks_failed++;
            std::cerr << "  Verify query failed: " << result.error_msg << std::endl;
        }
    };

    std::shared_ptr<MySql::DBTask> verify_task = std::make_shared<MySql::DBTask>();
    verify_task->sql = "SELECT id, name, age FROM test_users WHERE name = 'Alice'";
    verify_task->callback = verify_cb;
    thread_pool.SubmitTask(verify_task);

    // 等待所有任务完成（最多等 3 秒）
    std::cout << "\n[8] Waiting for tasks to complete..." << std::endl;
    for (int i = 0; i < 30; ++i) {
        if (tasks_completed.load() >= 6) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // ----------------------------------------------------------------
    // 8. 统计结果
    // ----------------------------------------------------------------
    std::cout << "\n[9] Summary:" << std::endl;
    std::cout << "  Total tasks completed: " << tasks_completed.load() << std::endl;
    std::cout << "  Failed tasks: " << tasks_failed.load() << std::endl;

    // ----------------------------------------------------------------
    // 9. 清理
    // ----------------------------------------------------------------
     std::cout << "\n[10] Cleaning up..." << std::endl;
    thread_pool.Stop();

    if (tasks_failed.load() == 0 && tasks_completed.load() >= 6) {
        std::cout << "\n=== ALL TESTS PASSED ===" << std::endl;
        return 0;
    } else {
        std::cout << "\n=== SOME TESTS FAILED ===" << std::endl;
        return 1;
    }

    return 0;
}


int Test_DB_Mysql::Test_Sync_Execute_Task()
{
    std::cout << "=== DB Thread Pool Test ===" << std::endl;
    // ----------------------------------------------------------------
    // 1. 初始化连接池
    // ----------------------------------------------------------------
    std::cout << "\n[1] Initializing connection pool..." << std::endl;
    std::unique_ptr<MySql::DBConnectionPool> pool;
    {
        pool = std::make_unique<MySql::DBConnectionPool>();
        if (!pool->Init("127.0.0.1", "root", "zzh@890918", "game_server", 3306, 5)) {
            std::cerr << "Failed to initialize connection pool" << std::endl;
            return 1;
        }

        std::cout << "  Idle connections: " << pool->IdleCount() << std::endl;
    } 
   

    // ----------------------------------------------------------------
    // 2. 初始化 DB 线程池（3 个工作线程）
    // ----------------------------------------------------------------
    std::cout << "\n[2] Initializing DB thread pool..." << std::endl;
    MySql::DBThreadPool thread_pool(std::move(pool), 3);
    std::cout << "  Thread pool ready." << std::endl;

    // ----------------------------------------------------------------
    // 3. 查询测试
    // ----------------------------------------------------------------
    std::cout << "\n[3] Querying data Before Update..." << std::endl;
    std::shared_ptr<MySql::DBTask> query_task = std::make_shared<MySql::DBTask>();
    query_task->sql = "SELECT id, name, age FROM test_users ORDER BY id";
    MySql::DBResult before_update_result = std::move(thread_pool.ExecuteSync(query_task));
    this->PrintResult(before_update_result);


    // ----------------------------------------------------------------
    // 4. 更新测试
    // ----------------------------------------------------------------
    std::cout << "\n[4] Updating data..." << std::endl;
    std::shared_ptr<MySql::DBTask> update_task = std::make_shared<MySql::DBTask>();
    update_task->sql = "UPDATE test_users SET age = 100 WHERE name = 'Alice'";
    MySql::DBResult update_result = std::move(thread_pool.ExecuteSync(update_task));
    this->PrintResult(update_result);

    // ----------------------------------------------------------------
    // 5. 查询测试
    // ----------------------------------------------------------------
    std::cout << "\n[5] Querying data after Update..." << std::endl;
    std::shared_ptr<MySql::DBTask> after_update_query_task = std::make_shared<MySql::DBTask>();
    after_update_query_task->sql = "SELECT id, name, age FROM test_users ORDER BY id";
    MySql::DBResult after_update_result = std::move(thread_pool.ExecuteSync(after_update_query_task));
    this->PrintResult(after_update_result);

    // ----------------------------------------------------------------
    // 9. 清理
    // ----------------------------------------------------------------
    std::cout << "\n[10] Cleaning up..." << std::endl;
    thread_pool.Stop();
    std::cout << "\n=== ALL TESTS PASSED ===" << std::endl;
    return 0;
}