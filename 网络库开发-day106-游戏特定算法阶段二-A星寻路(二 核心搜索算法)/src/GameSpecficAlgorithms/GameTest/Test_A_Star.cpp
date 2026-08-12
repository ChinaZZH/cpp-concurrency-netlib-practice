#include "Test_A_Star.h"
#include <iostream>
#include <cassert>
#include "../A_Star/GridMap.h"
#include "../A_Star/NodeManager.h"
#include "../A_Star/PathFinder.h"


void Test_A_Star::TestDataStruct_1()
{
    std::cout << "=== A* Day 1 Tests ===" << std::endl;
    TestGridMap();
    TestNodeManager();
    std::cout << "=== ALL TESTS PASSED ===" << std::endl;
}


void Test_A_Star::TestGridMap()
{
    std::cout << "[Test] GridMap..." << std::endl;

    A_Star::GridMap map(10, 10, 1);
    assert(map.GetWidth() == 10);
    assert(map.GetHeight() == 10);

    assert(map.IsWalkable(0, 0) == true);
    assert(map.IsWalkable(5, 5) == true);

    map.SetWalkable(3, 3, false);
    assert(map.IsWalkable(3, 3) == false);

    assert(map.IsValid(0, 0) == true);
    assert(map.IsValid(10, 0) == false);

    std::pair<int, int> world_pos = map.GridToWorld(0, 0);
    assert(world_pos.first == 0 && world_pos.second == 0);

    world_pos = map.GridToWorld(5, 5);
    assert(world_pos.first == 5 && world_pos.second == 5);

    std::pair<int, int> grid_pos = map.WorldToGrid(5, 5);
    assert(grid_pos.first == 5 && grid_pos.second == 5);

    grid_pos = map.WorldToGrid(9, 9);
    assert(grid_pos.first == 9 && grid_pos.second == 9);

    // LoadFromArray 测试
    std::vector<std::vector<bool>> walkable_data = {
        {true, true, false},
        {true, false, true},
        {true, true, true}
    };

    A_Star::GridMap map2;
    map2.Init(3, 3, 1);
    map2.LoadFromArray(walkable_data);
    assert(map2.IsWalkable(0, 0) == true);
    assert(map2.IsWalkable(2, 0) == false);
    assert(map2.IsWalkable(1, 1) == false);
    assert(map2.IsWalkable(2, 2) == true);

    std::cout << "[PASS] GridMap" << std::endl;
}

void Test_A_Star::TestNodeManager()
{
    std::cout << "[Test] NodeManager..." << std::endl;

    A_Star::NodeManager mgr(10, 10);
    assert(mgr.GetWidth() == 10);
    assert(mgr.GetHeight() == 10);

    A_Star::GridNode_Ptr node = mgr.GetNode(3, 5);
    assert(node != nullptr);
    assert(node->x == 3 && node->y == 5);

    node->src_to_cur = 10;
    node->final_total = 20;
    node->in_open_list = true;

    A_Star::GridNode_Ptr same_node = mgr.GetNode(3, 5);
    assert(same_node->src_to_cur == 10);
    assert(same_node->in_open_list == true);

    mgr.ResetAll();
    assert(same_node->src_to_cur == 0);
    assert(same_node->in_open_list == false);

    // 节点复用测试
    A_Star::GridNode_Ptr node2 = mgr.GetNode(3, 5);
    assert(node2->src_to_cur == 0);

    std::cout << "[PASS] NodeManager" << std::endl;
}


void Test_A_Star::Test_PathFinder()
{
    std::cout << "=== A* Day 2 Tests ===" << std::endl;
    TestBasicPath();
    TestWithObstacle();
    TestDiagonalPath();
    TestUnreachable();
    std::cout << "=== ALL TESTS PASSED ===" << std::endl;
}


void Test_A_Star::TestBasicPath() {
    std::cout << "[Test] Basic path..." << std::endl;

    std::shared_ptr<A_Star::GridMap> map = std::make_shared<A_Star::GridMap>(10, 10, 1.0f);
    std::shared_ptr<A_Star::NodeManager> nmgr = std::make_shared<A_Star::NodeManager>(10, 10);

    A_Star::PathFinder finder(map, nmgr);

    // 无障碍，从(0,0)到(9,9)
    //std::cout << "[Test] Basic path_111111" << std::endl;
    auto result = finder.FindPath(0, 0, 9, 9);
    //std::cout << "[Test] Basic path_222222" << std::endl;
    assert(result.found == true);
    assert(result.path.size() > 0);
    assert(result.path.front().first == 0 && result.path.front().second == 0);
    assert(result.path.back().first == 9 && result.path.back().second == 9);

    // 路径应该是最短的曼哈顿距离（18步）
    assert(result.path.size() == 19);  // 包含起点和终点

    std::cout << "[PASS] Basic path" << std::endl;
}

void Test_A_Star::TestWithObstacle() {
    std::cout << "[Test] Path with obstacle..." << std::endl;

    std::shared_ptr<A_Star::GridMap> map = std::make_shared<A_Star::GridMap>(10, 10, 1);
    std::shared_ptr<A_Star::NodeManager> nmgr = std::make_shared<A_Star::NodeManager>(10, 10);
    A_Star::PathFinder finder(map, nmgr);

    // 设置一堵墙：从(3,0)到(3,9)
    for (int y = 0; y < 5; ++y) {
        map->SetWalkable(3, y, false);
    }

    for (int y = 6; y < 10; ++y) {
        map->SetWalkable(3, y, false);
    }

    // 从(0,0)到(9,9)，必须绕墙
    auto result = finder.FindPath(0, 0, 9, 9);
    assert(result.found == true);
    assert(result.path.size() > 0);

    // 检查路径是否确实绕过了墙（路径中不应该出现 x=3 且 y 在 0-9 之间）
    for (const auto& pos : result.path) {
        assert(!(pos.first == 3 && pos.second >= 0 && pos.second < 5));
        assert(!(pos.first == 3 && pos.second >= 06 && pos.second < 10));
    }

    std::cout << "[PASS] Path with obstacle" << std::endl;
}


void Test_A_Star::TestDiagonalPath() {
    std::cout << "[Test] Diagonal path..." << std::endl;

    std::shared_ptr<A_Star::GridMap> map = std::make_shared<A_Star::GridMap>(10, 10, 1.0f);
    std::shared_ptr<A_Star::NodeManager> nmgr = std::make_shared<A_Star::NodeManager>(10, 10);
    A_Star::PathFinder finder(map, nmgr);

    auto result = finder.FindPath(0, 0, 9, 9, true);
    assert(result.found == true);
    // 对角线寻路路径应该比曼哈顿路径更短（9步 vs 18步）
    assert(result.path.size() < 19);

    std::cout << "[PASS] Diagonal path" << std::endl;
}


void Test_A_Star::TestUnreachable() {
    std::cout << "[Test] Unreachable path..." << std::endl;

    std::shared_ptr<A_Star::GridMap> map = std::make_shared<A_Star::GridMap>(10, 10, 1.0f);
    std::shared_ptr<A_Star::NodeManager> nmgr = std::make_shared<A_Star::NodeManager>(10, 10);
    A_Star::PathFinder finder(map, nmgr);

    // 用障碍物完全包围终点 (9,9)
    map->SetWalkable(8, 9, false);
    map->SetWalkable(9, 8, false);
    map->SetWalkable(8, 8, false);

    auto result = finder.FindPath(0, 0, 9, 9);
    assert(result.found == false);

    std::cout << "[PASS] Unreachable path" << std::endl;
}