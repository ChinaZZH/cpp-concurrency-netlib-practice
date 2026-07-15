#pragma once

#include <iostream>
#include <chrono>
#include <random>
#include <vector>
#include <memory>
#include <iomanip>

class IAOIManager;
class AOIPerformanceTest
{
public:
// 测试配置
    struct TestConfig 
    {
        int entityCount;        // 实体数量
        int moveCount;          // 移动次数
        int queryCount;         // 查询次数
        int worldWidth;
        int worldHeight;
        int gridSize;
        int radius;             // 九宫格半径 
    };


        // 测试结果
    struct TestResult {
        std::string name;
        double insertTimeMs;    // 插入耗时 (ms)
        double moveTimeMs;      // 移动耗时 (ms)
        double queryTimeMs;     // 查询耗时 (ms)
        double totalTimeMs;     // 总耗时
    };

    int PerformanceTest();

private:
        // 生成随机位置
    std::pair<int, int> RandomPosition(std::mt19937& gen, int w, int h);

    // 运行单个算法的测试
    TestResult RunTest(IAOIManager& aoi, const TestConfig& cfg, std::mt19937& gen);

    // 打印测试结果
    void PrintResult(const TestResult& result);

    

};



