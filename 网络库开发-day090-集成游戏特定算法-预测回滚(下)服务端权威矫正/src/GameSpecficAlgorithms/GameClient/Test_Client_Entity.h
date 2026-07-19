#pragma once

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

class TestClientEntity
{
public:
    TestClientEntity() = default;
    ~TestClientEntity() = default;
    int Test();

private:
    void DummySend(uint32_t conn_id, const std::string& data);
    bool TestDirtyTracker();
    bool TestEntityHistory();
    bool TestReassemblyBuffer();
    bool TestUdpReassemblyBuffer();
};