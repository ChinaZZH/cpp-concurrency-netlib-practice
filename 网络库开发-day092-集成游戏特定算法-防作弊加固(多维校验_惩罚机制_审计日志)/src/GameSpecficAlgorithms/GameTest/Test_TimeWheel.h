#pragma once

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

class Test_TimeWheel
{
public:
    Test_TimeWheel() = default;
    
    ~Test_TimeWheel() = default;
    
    void TestAll();

private:
    void TestSingleTimer();

    void TestRepeatedTimer();
    
    void TestCancelTimer();
    
    void TestMultipleTimers();
    
    void TestInfiniteLoopTimer();

private:
    std::atomic<int> counter1{0};
    std::atomic<int> counter2{0};
    std::atomic<int> counter3{0};
};