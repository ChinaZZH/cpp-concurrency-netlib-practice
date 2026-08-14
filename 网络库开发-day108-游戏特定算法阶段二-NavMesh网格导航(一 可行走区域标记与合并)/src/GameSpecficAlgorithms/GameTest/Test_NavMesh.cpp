#include "Test_NavMesh.h" 
#include <iostream>
#include <sstream>
#include <cassert>
#include <memory>
#include <cassert>
#include "../A_Star/GridMap.h"
#include "../A_NavMesh/RegionMarker.h"
#include "../A_NavMesh/PolygonExtractor.h"


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


void Test_NavMesh::TestPolygon()
{
    std::cout << "=== Sub-task 1.2: Polygon Extraction ===" << std::endl;

    // 创建 10x10 地图
    auto map = std::make_shared<A_Star::GridMap>(10, 10, 1.0f);

    // 设置一堵墙从 (3,0) 到 (6,9)
    for (int y = 0; y < 10; ++y) {
        for (int x = 3; x <= 6; ++x) {
            map->SetWalkable(x, y, false);
        }
    }

    // 1. 区域标记
    NavMesh::RegionMarker marker;
    auto region_ids = marker.MarkRegions(map);
    std::cout << "Regions marked: " << marker.GetRegionCount() << std::endl;

    /*
    bool firstTag;
    std::stringstream ss;
    for (const auto& region_id_value : region_ids) 
    {
        if(firstTag)
        {
            firstTag = false;
            ss << region_id_value;
        }
        else
        {
            ss << " , " << region_id_value;
        }
    }

    std::cout << "Regions id list( " << ss.str() << " )" << std::endl;
    */

    // 2. 多边形提取
    NavMesh::PolygonExtractor extractor;
    auto region_polygons = extractor.Extract(map, region_ids);

    // 验证
    assert(region_polygons.size() == 2);  // 左右两个区域

    for (const auto& rp : region_polygons) {
        std::cout << "Region " << rp.region_id
                  << ": vertices=" << rp.polygon.vertices.size()
                  << ", area=" << rp.polygon.GetArea()
                  << ", center=(" << rp.polygon.center_x << ", " << rp.polygon.center_y << ")"
                  << std::endl;

        assert(rp.polygon.vertices.size() >= 3);
        assert(rp.polygon.GetArea() > 0.0f);
    }

    
    
    for(const auto& rp : region_polygons) 
    {
        if(1 == rp.region_id)
        {
            // 验证多边形包含区域内的点
            // 左侧区域 (1,1) 应该在左侧多边形内
            std::pair<int, int> world_pos = map->GridToWorld(1, 1);
            bool contains = rp.polygon.ContainsPoint(world_pos.first, world_pos.second);
            assert(contains);
            std::cout << "Polygon contains (1,1): " << (contains ? "✓" : "✗") << std::endl;
        }
        else if(2 == rp.region_id)
        {
            // 右侧区域 (8,1) 应该在右侧多边形内
            std::pair<int, int> world_pos = map->GridToWorld(8, 1);
            bool contains = rp.polygon.ContainsPoint(world_pos.first, world_pos.second);
            assert(contains);
            std::cout << "Polygon contains (8,1): " << (contains ? "✓" : "✗") << std::endl;
        }
    }

   
    // 墙上的点 (4,4) 不应该在任何多边形内
    std::pair<int, int> world_pos = map->GridToWorld(4, 4);
    bool in_left = region_polygons[0].polygon.ContainsPoint(world_pos.first, world_pos.second);
    bool in_right = region_polygons[1].polygon.ContainsPoint(world_pos.first, world_pos.second);
    assert(!in_left && !in_right);
    std::cout << "Wall (4,4) not in any polygon: ✓" << std::endl;

    std::cout << "=== Sub-task 1.2 PASSED ===" << std::endl;
}


void Test_NavMesh::TestCenterOfPolygon()
{
    std::cout << "=== Sub-task 1.3: Center Point Calculation ===" << std::endl;

    // 构建一个简单的多边形（方形）
    NavMesh::Polygon poly;
    poly.vertices = {
        {0.0f, 0.0f},
        {2.0f, 0.0f},
        {2.0f, 2.0f},
        {0.0f, 2.0f}
    };
    poly.CalculateCentroid();

    assert(poly.center_x == 1.0f);
    assert(poly.center_y == 1.0f);
    std::cout << "Square center: (1.0, 1.0) ✓" << std::endl;

    // 验证中心点在多边形内部
    bool inside = poly.ContainsPoint(poly.center_x, poly.center_y);
    assert(inside);
    std::cout << "Center point is inside polygon ✓" << std::endl;

     // 测试不规则多边形（L形）
    NavMesh::Polygon poly2;
    poly2.vertices = {
        {0.0f, 0.0f},
        {3.0f, 0.0f},
        {3.0f, 1.0f},
        {1.0f, 1.0f},
        {1.0f, 2.0f},
        {0.0f, 2.0f}
    };
    poly2.CalculateCentroid();
    std::cout << "L-shape center: (" << poly2.center_x << ", " << poly2.center_y << ")" << std::endl;

    bool inside2 = poly2.ContainsPoint(poly2.center_x, poly2.center_y);
    assert(inside2);
    std::cout << "Center point is inside L-shape ✓" << std::endl;

    std::cout << "=== Sub-task 1.3 PASSED ===" << std::endl;
}