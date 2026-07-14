
#include "Test_TimeWheel.h"
#include "../Common/TimeWheel.h"

void Test_TimeWheel::TestAll() {
    std::cout << "========== TimeWheel Test Suite ==========" << std::endl << std::endl;

    TestSingleTimer();
    TestRepeatedTimer();
    TestCancelTimer();
    TestMultipleTimers();
    TestInfiniteLoopTimer();

    std::cout << "========== All tests completed ==========" << std::endl;
}


void Test_TimeWheel::TestSingleTimer() 
{
    std::cout << "=== Test 1: Single timer (3 seconds) ===" << std::endl;
    TimeWheel tw;

    uint64_t id = tw.AddNewTimer(3, 1, [this]() {
        std::cout << "Single timer triggered after 3 seconds" << std::endl;
        counter1++;
    });

    // 模拟驱动时间轮运行 5 秒
    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        tw.RunNexTick();
        std::cout << "Tick " << i+1 << " passed" << std::endl;
    }

    if (counter1 == 1) {
        std::cout << "✅ PASS: Single timer triggered once" << std::endl;
    } else {
        std::cout << "❌ FAIL: Single timer triggered " << counter1 << " times" << std::endl;
    }
    std::cout << std::endl;
}


void Test_TimeWheel::TestRepeatedTimer() 
{
    std::cout << "=== Test 2: Repeated timer (every 2 seconds, 3 times) ===" << std::endl;
    TimeWheel tw;

    uint64_t id = tw.AddNewTimer(2, 3, [this]() {
        counter2++;
        std::cout << "Repeated timer triggered, count: " << counter2 << std::endl;
    });

    // 运行 8 秒，应该触发 3 次（第2、4、6秒）
    for (int i = 0; i < 8; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        tw.RunNexTick();
    }

    if (counter2 == 3) {
        std::cout << "✅ PASS: Repeated timer triggered 3 times" << std::endl;
    } else {
        std::cout << "❌ FAIL: Repeated timer triggered " << counter2 << " times" << std::endl;
    }
    std::cout << std::endl;
}

void Test_TimeWheel::TestCancelTimer() 
{
    std::cout << "=== Test 3: Cancel timer ===" << std::endl;
    TimeWheel tw;

    uint64_t id = tw.AddNewTimer(2, 5, [this]() {
        counter3++;
        std::cout << "Timer triggered (should be canceled before trigger)" << std::endl;
    });

    // 先取消定时器
    std::cout << "Canceling timer..." << std::endl;
    tw.CancelTimer(id);

    // 运行 5 秒
    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        tw.RunNexTick();
    }

    if (counter3 == 0) {
        std::cout << "✅ PASS: Timer canceled, never triggered" << std::endl;
    } else {
        std::cout << "❌ FAIL: Timer triggered " << counter3 << " times" << std::endl;
    }
    std::cout << std::endl;
}

void Test_TimeWheel::TestMultipleTimers() 
{
    std::cout << "=== Test 4: Multiple timers at different delays ===" << std::endl;
    TimeWheel tw;

    std::atomic<int> a{0}, b{0}, c{0};

    tw.AddNewTimer(1, 1, [&]() { a++; std::cout << "Timer A (1s) triggered" << std::endl; });
    tw.AddNewTimer(3, 1, [&]() { b++; std::cout << "Timer B (3s) triggered" << std::endl; });
    tw.AddNewTimer(5, 1, [&]() { c++; std::cout << "Timer C (5s) triggered" << std::endl; });

    for (int i = 0; i < 6; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        tw.RunNexTick();
        std::cout << "Tick " << i+1 << " done" << std::endl;
    }

    bool ok = (a == 1 && b == 1 && c == 1);
    if (ok) {
        std::cout << "✅ PASS: All three timers triggered correctly" << std::endl;
    } else {
        std::cout << "❌ FAIL: A=" << a << ", B=" << b << ", C=" << c << std::endl;
    }
    std::cout << std::endl;
}

void Test_TimeWheel::TestInfiniteLoopTimer() 
{
    std::cout << "=== Test 5: Infinite loop timer (remainCount = 0) ===" << std::endl;
    TimeWheel tw;
    std::atomic<int> count{0};

    uint64_t id = tw.AddNewTimer(2, 0, [&]() {
        count++;
        std::cout << "Infinite timer triggered, count: " << count << std::endl;
    });

    // 运行 6 秒，应该触发 3 次（第2、4、6秒）
    for (int i = 0; i < 6; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        tw.RunNexTick();
    }

    tw.CancelTimer(id);

    if (count >= 3) {
        std::cout << "✅ PASS: Infinite timer triggered " << count << " times (>= 3)" << std::endl;
    } else {
        std::cout << "❌ FAIL: Infinite timer triggered " << count << " times" << std::endl;
    }
    std::cout << std::endl;
}

