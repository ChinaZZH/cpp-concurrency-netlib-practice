#pragma once

#include <iostream>
#include <chrono>
#include <memory>
#include "../FrameSync/ServerPlayerManager.h"
#include "../../../build/proto_gen/game_event.pb.h"

//class AOIManager;
class CrossListAOI;
class GridAOI;
class IAOIManager;
class GameServer;

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
    //void TestRemotePlayerSmoother();

    void TestEventSinkLog();

    void TestEventStore();
    
private:
    void PrintNeighbors(std::shared_ptr<IAOIManager> aoi, int id);
    
    void PrintState(const char* label, const ServerPlayerState& state);

    GameEvent MakeTestEvent(uint32_t player_id, uint32_t server_frame, EventType type, uint32_t target_id = 0, int32_t damage = 0);

    void PrintEvent(const GameEvent& event);
};