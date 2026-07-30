#pragma once

#include <iostream>
#include <chrono>
#include <memory>
#include "../FrameSync/ServerPlayerManager.h"
#include "../../../build/proto_gen/game_event.pb.h"
#include "../BehaviorTree/BehaviorTree.h"

//class AOIManager;
class CrossListAOI;
class GridAOI;
class IAOIManager;
class GameServer;
class RankManager;
class StateMachine;
struct StateContext;

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
    
    void TestRankMgr();

    void TestFsm();

    void TestBehaviorTree_Step2();
    void TestBehaviorTree_Step4();

private:
    void PrintNeighbors(std::shared_ptr<IAOIManager> aoi, int id);
    
    void PrintState(const char* label, const ServerPlayerState& state);

    GameEvent MakeTestEvent(uint32_t player_id, uint32_t server_frame, EventType type, uint32_t target_id = 0, int32_t damage = 0);

    void PrintEvent(const GameEvent& event);

    void PrintTopN(const RankManager& mgr, uint32_t n);

    void PrintStatus(const StateMachine& fsm, const StateContext& ctx, const char* msg = "");

    void PrintResult_ForBehaviorTree(const std::string& test_name, BTStatus status);
};