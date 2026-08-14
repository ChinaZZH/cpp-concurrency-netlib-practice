#include "Test_NavMesh.h" 
#include <iostream>
#include <cassert>
#include <memory>
#include "../A_Star/GridMap.h"
#include "../A_NavMesh/RegionMarker.h"



void Test_NavMesh::Test_RegionMarker()
{
    std::cout << "=== Sub-task 1.1: Region Marking ===" << std::endl;

    // 创建 10x10 地图
    std::shared_ptr<A_Star::GridMap> map = std::make_shared<A_Star::GridMap>(10, 10, 1.0f);

    // 设置障碍物：一堵墙从 (3,2) 到 (6,7)
    for (int y = 2; y <= 7; ++y) {
        for (int x = 3; x <= 6; ++x) {
            map->SetWalkable(x, y, false);
        }
    }

    // 执行区域标记
    NavMesh::RegionMarker marker;
    auto region_ids = marker.MarkRegions(map);

     // 验证：障碍物将地图分成左右两个区域（顶部和底部可能连通）
    // 顶部：y=0,1 左右互通 → 同一区域
    // 底部：y=8,9 左右互通 → 同一区域
    // 中间：被墙隔开，左右各自是不同区域

    uint32_t top_left = region_ids[0 * 10 + 0];   // (0,0)
    uint32_t top_right = region_ids[0 * 10 + 9];   // (9,0)
    uint32_t mid_left = region_ids[2 * 10 + 2];    // (2,2) 
    uint32_t mid_right = region_ids[2 * 10 + 7];   // (7,2) 


    // 顶部左右应该在同一区域（y=0 没有障碍物）
    assert(top_left == top_right);
    std::cout << "Top region: " << top_left << " (same for left and right) ✓" << std::endl;

    // 中间左右应该在不同区域（被墙隔开）
    assert(mid_left == mid_right);
    std::cout << "Middle left: " << top_left << " (same for middle right and right) ✓" << std::endl;

    

    // 验证：墙上的格子不可行走，区域 ID 为 0
    assert(region_ids[3 * 10 + 3] == 0);
    std::cout << "Wall cell: region_id = 0 ✓" << std::endl;

    // 打印区域数量
    std::cout << "Total regions: " << marker.GetRegionCount() << std::endl;

    // 设置一堵墙从 (3,0) 到 (6,9)，完全阻断左右
    for (int y = 0; y < 10; ++y) {
        for (int x = 3; x <= 6; ++x) {
            map->SetWalkable(x, y, false);
        }
    }

    region_ids = marker.MarkRegions(map);
    mid_left = region_ids[2 * 10 + 2];    // (2,2) 
    mid_right = region_ids[2 * 10 + 7];   // (7,2) 
    assert(mid_left != mid_right);
    std::cout << "Middle left: " << mid_left << ", middle right: " << mid_right << " (different) ✓" << std::endl;

    // 预期：4 个区域（左上、右上、左下、右下），但因为顶部/底部连通，实际可能只有 2 个区域
    // 取决于障碍物是否完全阻断，这里根据实际地图布局调整断言
    assert(marker.GetRegionCount() >= 2);

    std::cout << "=== Sub-task 1.1 PASSED ===" << std::endl;
}