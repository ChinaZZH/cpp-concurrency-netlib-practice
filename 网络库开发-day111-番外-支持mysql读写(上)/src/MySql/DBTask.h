#pragma once
#include <mysql/mysql.h>
#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace MySql
{
    // 查询结果封装
    struct DBResult
    {
        bool success = false;
        std::string error_msg;
        std::vector<std::vector<std::string>> rows;     // 查询结果行
        std::vector<std::string> columns;               // 列名
        uint64_t affected_rows = 0;
        uint64_t insert_id = 0;
    };


    // 任务结构
    struct DBTask
    {
        std::string sql;
        std::function<void(const DBResult&)> callback;
        uint32_t conn_id;            // 哪个客户端发起的请求（用于回包）
        uint32_t request_id;         // 用于匹配请求/响应（可选）
    };
}