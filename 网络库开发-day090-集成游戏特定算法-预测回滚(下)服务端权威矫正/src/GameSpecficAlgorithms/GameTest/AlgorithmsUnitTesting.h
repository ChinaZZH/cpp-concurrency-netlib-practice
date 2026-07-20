#pragma once

#include <iostream>
#include <chrono>
#include <memory>

//class AOIManager;
class CrossListAOI;
class GridAOI;
class IAOIManager;
class AlgorithmsUnitTesting
{
public:
    AlgorithmsUnitTesting();

public:
    void TestAoiManager();  // 测试aoi算法九宫格
    void TestQuadTreeAOI();

    void TestDeterminism();
    void TestFullDeterminism();

    void TestServerPlayerManger();
    
private:
    void PrintNeighbors(std::shared_ptr<IAOIManager> aoi, int id);
    
};