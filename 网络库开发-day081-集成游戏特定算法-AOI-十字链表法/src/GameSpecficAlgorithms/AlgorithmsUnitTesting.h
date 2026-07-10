#pragma once

#include <iostream>
#include <chrono>

class AOIManager;
class CrossListAOI;
class AlgorithmsUnitTesting
{
public:
    AlgorithmsUnitTesting();

public:
    void TestAoiManager();  // 测试aoi算法九宫格

private:
    void PrintNeighbors(const CrossListAOI& aoi, int id);
};