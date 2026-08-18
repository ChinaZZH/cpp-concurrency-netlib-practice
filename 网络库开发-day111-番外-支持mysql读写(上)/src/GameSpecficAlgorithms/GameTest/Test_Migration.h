#pragma once


//#include <iostream>
//#include <cassert>
//#include <thread>
//#include <chrono>

//#include "../DynamicPartition/PartitionManager.h"
//#include "../DynamicPartition/MigrationManager.h"
#include "../DynamicPartition/MigrationManager.h"

class Test_Migration
{
public:
    void TestMigrationAll();


private:
    void TestMigrationStateMachine();

    void TestSerialization();

    void TestDeserializationAndRestore();

    void TestSourceCleanup();

    void TestRollback();

    void TestEndToEndMigration();

private:
    void MockSendToTarget(const MigrationData& data);

    bool MockReceiveFromSource(MigrationData& out);

private:
    // ===== 辅助：模拟网络传输 =====
    // 在单进程测试中，用一个全局变量模拟"网络传输" 
    MigrationData g_mock_network_data;
    
    bool g_mock_network_has_data = false;
};



