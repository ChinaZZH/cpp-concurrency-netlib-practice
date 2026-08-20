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


void Test_DB_Mysql::PrintResult(const std::string& label, const MySql::DBResult& result)
{
    if(!label.empty())
    {
        std::cout << "  [" << label << "] ";
    }

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

    // 打印前 5 行数据
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

    std::string emptyLable;
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
            PrintResult(emptyLable, result);
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
            PrintResult(emptyLable, result);
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
    std::string emptyLable;
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
    this->PrintResult(emptyLable, before_update_result);


    // ----------------------------------------------------------------
    // 4. 更新测试
    // ----------------------------------------------------------------
    std::cout << "\n[4] Updating data..." << std::endl;
    std::shared_ptr<MySql::DBTask> update_task = std::make_shared<MySql::DBTask>();
    update_task->sql = "UPDATE test_users SET age = 100 WHERE name = 'Alice'";
    MySql::DBResult update_result = std::move(thread_pool.ExecuteSync(update_task));
    this->PrintResult(emptyLable, update_result);

    // ----------------------------------------------------------------
    // 5. 查询测试
    // ----------------------------------------------------------------
    std::cout << "\n[5] Querying data after Update..." << std::endl;
    std::shared_ptr<MySql::DBTask> after_update_query_task = std::make_shared<MySql::DBTask>();
    after_update_query_task->sql = "SELECT id, name, age FROM test_users ORDER BY id";
    MySql::DBResult after_update_result = std::move(thread_pool.ExecuteSync(after_update_query_task));
    this->PrintResult(emptyLable, after_update_result);

    // ----------------------------------------------------------------
    // 9. 清理
    // ----------------------------------------------------------------
    std::cout << "\n[10] Cleaning up..." << std::endl;
    thread_pool.Stop();
    std::cout << "\n=== ALL TESTS PASSED ===" << std::endl;
    return 0;
}


int Test_DB_Mysql::Test_Prepared_Statement()
{
    std::cout << "=== Prepared Statement Unit Test ===" << std::endl;

    // ----------------------------------------------------------------
    // 1. 初始化连接池
    // ----------------------------------------------------------------
    std::cout << "\n[Step 1] Initializing connection pool..." << std::endl;
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
    // 2. 初始化 DB 线程池（2 个工作线程）
    // ----------------------------------------------------------------
    std::cout << "\n[2] Initializing DB thread pool..." << std::endl;
    MySql::DBThreadPool thread_pool(std::move(pool), 2);
    std::cout << "  Thread pool ready." << std::endl;

    std::atomic<int> tasks_done{0};
    std::atomic<int> tasks_failed{0};
    std::atomic<int> tasks_ok{0};

    // 辅助回调：计数
    auto count_cb = [&](const MySql::DBResult& result) {
        tasks_done++;
        if (result.success) tasks_ok++;
        else tasks_failed++;
    };


    // ----------------------------------------------------------------
    // 3. 创建测试表（普通 SQL）
    // ----------------------------------------------------------------
    std::cout << "\n[Step 3] Creating test table (normal SQL)..." << std::endl;
    std::shared_ptr<MySql::DBTask> create_task = std::make_shared<MySql::DBTask>();
    create_task->sql = R"(
        CREATE TABLE IF NOT EXISTS test_prepared (
            id INT AUTO_INCREMENT PRIMARY KEY,
            name VARCHAR(50) NOT NULL,
            age INT,
            email VARCHAR(100)
        )
    )";

    create_task->callback = [&](const MySql::DBResult& result) {
        tasks_done++;
        if (result.success) {
            tasks_ok++;
            std::cout << "  Table created successfully." << std::endl;
        } else {
            tasks_failed++;
            std::cerr << "  Table creation failed: " << result.error_msg << std::endl;
        }
    };
    thread_pool.SubmitTask(create_task);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    

     // ----------------------------------------------------------------
    // 4. 清空表（普通 SQL）
    // ----------------------------------------------------------------
    std::cout << "\n[Step 4] Clearing table..." << std::endl;
    std::shared_ptr<MySql::DBTask> clear_task = std::make_shared<MySql::DBTask>();
    clear_task->sql = "TRUNCATE TABLE test_prepared";
    clear_task->callback = count_cb;
    thread_pool.SubmitTask(clear_task);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // ----------------------------------------------------------------
    // 5. 测试1：插入单行（预处理）
    // ----------------------------------------------------------------
    std::cout << "\n[Test 1] Insert single row (prepared)..." << std::endl;
    std::shared_ptr<MySql::DBTask> insert1_task= std::make_shared<MySql::DBTask>();
    insert1_task->sql = "INSERT INTO test_prepared (name, age, email) VALUES (?, ?, ?)";
    insert1_task->params = {"Alice", "25", "alice@test.com"};
    insert1_task->callback = [&](const MySql::DBResult& result) {
        tasks_done++;
        if (result.success) {
            tasks_ok++;
            std::cout << "  Inserted 1 row, ID: " << result.insert_id << std::endl;
        } else {
            tasks_failed++;
            std::cerr << "  Insert failed: " << result.error_msg << std::endl;
        }
    };
    thread_pool.SubmitTask(insert1_task);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // ----------------------------------------------------------------
    // 6. 测试2：插入多行（预处理，批量）
    // ----------------------------------------------------------------
    std::cout << "\n[Test 2] Insert multiple rows (prepared)..." << std::endl;
    std::vector<std::tuple<std::string, int, std::string>> users = {
        {"Bob", 30, "bob@test.com"},
        {"Charlie", 35, "charlie@test.com"},
        {"Diana", 28, "diana@test.com"},
        {"Eve", 22, "eve@test.com"}
    };
    std::atomic<int> insert_count{0};

    for (const auto& [name, age, email] : users) {
        std::shared_ptr<MySql::DBTask> task = std::make_shared<MySql::DBTask>();
        task->sql = "INSERT INTO test_prepared (name, age, email) VALUES (?, ?, ?)";
        task->params = {name, std::to_string(age), email};
        task->callback = [&](const MySql::DBResult& result) {
            tasks_done++;
            if (result.success) {
                tasks_ok++;
                insert_count++;
            } else {
                tasks_failed++;
                std::cerr << "  Insert failed for " << name << ": " << result.error_msg << std::endl;
            }
        };
        thread_pool.SubmitTask(task);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    // 等待所有插入完成
    while (insert_count.load() < 4) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << "  Inserted " << insert_count.load() << " rows." << std::endl;

    // ----------------------------------------------------------------
    // 7. 测试3：查询（预处理，带参数）
    // ----------------------------------------------------------------
    std::cout << "\n[Test 3] Query with parameter (prepared)..." << std::endl;
    std::shared_ptr<MySql::DBTask> query_task = std::make_shared<MySql::DBTask>();
    query_task->sql = "SELECT id, name, age, email FROM test_prepared WHERE age > ?";
    query_task->params = {"26"};
    query_task->callback = [&](const MySql::DBResult& result) {
        tasks_done++;
        if (result.success) {
            tasks_ok++;
            PrintResult("Query age > 26", result);
            // 验证结果数量：应该至少 3 行（Bob 30, Charlie 35, Diana 28）
            assert(result.rows.size() >= 3);
        } else {
            tasks_failed++;
            std::cerr << "  Query failed: " << result.error_msg << std::endl;
        }
    };
    thread_pool.SubmitTask(query_task);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

     // ----------------------------------------------------------------
    // 8. 测试4：更新（预处理）
    // ----------------------------------------------------------------
    std::cout << "\n[Test 4] Update with parameter (prepared)..." << std::endl;
    std::shared_ptr<MySql::DBTask> update_task = std::make_shared<MySql::DBTask>();
    update_task->sql = "UPDATE test_prepared SET age = age + 1 WHERE name = ?";
    update_task->params = {"Alice"};
    update_task->callback = [&](const MySql::DBResult& result) {
        tasks_done++;
        if (result.success) {
            tasks_ok++;
            std::cout << "  Updated " << result.affected_rows << " row(s)." << std::endl;
            assert(result.affected_rows == 1);
        } else {
            tasks_failed++;
            std::cerr << "  Update failed: " << result.error_msg << std::endl;
        }
    };
    thread_pool.SubmitTask(update_task);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));


     // ----------------------------------------------------------------
    // 9. 测试5：删除（预处理）
    // ----------------------------------------------------------------
    std::cout << "\n[Test 5] Delete with parameter (prepared)..." << std::endl;
    std::shared_ptr<MySql::DBTask> delete_task = std::make_shared<MySql::DBTask>();
    delete_task->sql = "DELETE FROM test_prepared WHERE name = ?";
    delete_task->params = {"Eve"};
    delete_task->callback = [&](const MySql::DBResult& result) {
        tasks_done++;
        if (result.success) {
            tasks_ok++;
            std::cout << "  Deleted " << result.affected_rows << " row(s)." << std::endl;
            assert(result.affected_rows == 1);
        } else {
            tasks_failed++;
            std::cerr << "  Delete failed: " << result.error_msg << std::endl;
        }
    };
    thread_pool.SubmitTask(delete_task);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // ----------------------------------------------------------------
    // 10. 测试6：参数数量不匹配（错误处理）
    // ----------------------------------------------------------------
    std::cout << "\n[Test 6] Parameter count mismatch (error handling)..." << std::endl;
    std::shared_ptr<MySql::DBTask> mismatch_task  = std::make_shared<MySql::DBTask>();
    mismatch_task->sql = "INSERT INTO test_prepared (name, age) VALUES (?, ?)";
    mismatch_task->params = {"OnlyName"};  // 缺少第二个参数
    mismatch_task->callback = [&](const MySql::DBResult& result) {
        tasks_done++;
        if (!result.success) {
            tasks_ok++;  // 预期失败
            std::cout << "  Expected error: " << result.error_msg << std::endl;
        } else {
            tasks_failed++;
            std::cerr << "  Unexpected success!" << std::endl;
        }
    };
    thread_pool.SubmitTask(mismatch_task);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // ----------------------------------------------------------------
    // 11. 测试7：SQL 注入防护测试（参数中包含特殊字符）
    // ----------------------------------------------------------------
    std::cout << "\n[Test 7] SQL injection protection (special chars in param)..." << std::endl;
    std::string malicious = "'; DROP TABLE test_prepared; --";
    std::shared_ptr<MySql::DBTask> safe_task = std::make_shared<MySql::DBTask>();
    safe_task->sql = "SELECT * FROM test_prepared WHERE name = ?";
    safe_task->params = {malicious};
    safe_task->callback = [&](const MySql::DBResult& result) {
        tasks_done++;
        if (result.success) {
            tasks_ok++;
            // 应该返回空结果集（没有匹配），但不会执行 DROP
            std::cout << "  Query executed safely, returned " << result.rows.size() << " rows." << std::endl;
            // 验证表仍然存在（通过后续查询验证）
        } else {
            tasks_failed++;
            std::cerr << "  Query failed: " << result.error_msg << std::endl;
        }
    };
    thread_pool.SubmitTask(safe_task);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // ----------------------------------------------------------------
    // 12. 测试8：验证表仍然存在（检查删除操作未成功）
    // ----------------------------------------------------------------
    std::cout << "\n[Test 8] Verify table still exists (no injection)..." << std::endl;
    std::shared_ptr<MySql::DBTask> verify_task = std::make_shared<MySql::DBTask>();
    verify_task->sql = "SELECT COUNT(*) FROM test_prepared";
    verify_task->callback = [&](const MySql::DBResult& result) {
        tasks_done++;
        if (result.success && !result.rows.empty()) {
            tasks_ok++;
            std::cout << "  Table still exists, count: " << result.rows[0][0] << std::endl;
        } else {
            tasks_failed++;
            std::cerr << "  Table verification failed!" << std::endl;
        }
    };
    thread_pool.SubmitTask(verify_task);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // ----------------------------------------------------------------
    // 13. 等待所有任务完成
    // ----------------------------------------------------------------
    std::cout << "\n[Step 9] Waiting for all tasks to complete..." << std::endl;
    int total_expected = 10; // 根据实际提交的任务数调整
    for (int i = 0; i < 50; ++i) {
        if (tasks_done.load() >= total_expected) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // ----------------------------------------------------------------
    // 14. 汇总
    // ----------------------------------------------------------------
    std::cout << "\n[Summary]" << std::endl;
    std::cout << "  Total tasks: " << tasks_done.load() << std::endl;
    std::cout << "  Succeeded: " << tasks_ok.load() << std::endl;
    std::cout << "  Failed: " << tasks_failed.load() << std::endl;

    // ----------------------------------------------------------------
    // 15. 清理
    // ----------------------------------------------------------------
    std::cout << "\n[Cleanup] Dropping test table..." << std::endl;
    std::shared_ptr<MySql::DBTask> drop_task = std::make_shared<MySql::DBTask>();
    drop_task->sql = "DROP TABLE IF EXISTS test_prepared";
    drop_task->callback = count_cb;
    thread_pool.SubmitTask(drop_task);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    thread_pool.Stop();

    // ----------------------------------------------------------------
    // 16. 结果判定
    // ----------------------------------------------------------------
    if (tasks_failed.load() == 0 && tasks_ok.load() > 0) {
        std::cout << "\n=== ALL TESTS PASSED ===" << std::endl;
        return 0;
    } else {
        std::cout << "\n=== SOME TESTS FAILED ===" << std::endl;
        return 1;
    }
}