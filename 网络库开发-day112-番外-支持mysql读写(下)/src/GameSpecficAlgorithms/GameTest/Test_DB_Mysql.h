#pragma once
#include "../../MySql/DBTask.h"

class Test_DB_Mysql
{
public:
    int Test_Connection_Pool();

    int Test_DB_Task_Pool();
    
    // 测试同步的情况
    int Test_Sync_Execute_Task();

    int Test_Prepared_Statement();

private:
    void PrintResult(const std::string& label, const MySql::DBResult& result);
};