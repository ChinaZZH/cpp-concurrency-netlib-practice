#pragma once
#include "../../MySql/DBTask.h"

class Test_DB_Mysql
{
public:
    int Test_Connection_Pool();

    int Test_DB_Task_Pool();
    
private:
    void PrintResult(const MySql::DBResult& result);
};