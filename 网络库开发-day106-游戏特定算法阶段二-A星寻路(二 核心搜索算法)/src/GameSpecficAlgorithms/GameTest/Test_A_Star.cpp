#include "Test_A_Star.h"
#include <iostream>
#include <cassert>
#include "../A_Star/GridMap.h"
#include "../A_Star/NodeManager.h"

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