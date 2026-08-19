#pragma once
#include "../../MySql/DBTask.h"

class Test_DB_Mysql
{
public:
    int Test_Connection_Pool();

    int Test_DB_Task_Pool();
    
    // 测试同步的情况
    int Test_Sync_Execute_Task();

private:
    void PrintResult(const MySql::DBResult& result);
};