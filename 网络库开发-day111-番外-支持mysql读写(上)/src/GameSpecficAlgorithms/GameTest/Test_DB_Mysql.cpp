#include "Test_DB_Mysql.h"
#include <iostream>
#include <mysql/mysql.h>
#include "../../MySql/DBConnectionPool.h"

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