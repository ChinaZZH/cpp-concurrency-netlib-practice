#pragma once

#include <iostream>
#include <chrono>
#include <memory>
#include "../FrameSync/ServerPlayerManager.h"

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
    void TestRemotePlayerSmoother();
private:
    void PrintNeighbors(std::shared_ptr<IAOIManager> aoi, int id);
    void PrintState(const char* label, const ServerPlayerState& state);
};