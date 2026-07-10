#include "AlgorithmsUnitTesting.h"
#include "AOI/AOIManager.h"
#include "AOI/CrossListAOI.h"

 AlgorithmsUnitTesting::AlgorithmsUnitTesting()
 {

 }

 // 测试aoi算法九宫格
void AlgorithmsUnitTesting::TestAoiManager()
{
    //AOIManager aoi(100);
    CrossListAOI aoi(100);

    aoi.AddEntity(1, 50, 50);
    aoi.AddEntity(2, 120, 50);
    aoi.AddEntity(3, 50, 120);
    aoi.AddEntity(4, 200, 200);    
    PrintNeighbors(aoi, 1); // 应该在九宫格内看到 2, 3

    // 移动实体
    aoi.MoveEntity(2, 100, 50);
    PrintNeighbors(aoi, 1); // 2 可能仍在九宫格内

    aoi.MoveEntity(2, 300, 300);
    PrintNeighbors(aoi, 1); // 2 应该离开九宫格

    // 删除实体
    aoi.RemoveEntity(3);
    PrintNeighbors(aoi, 1);
}

void AlgorithmsUnitTesting::PrintNeighbors(const CrossListAOI& aoi, int id)
{
    auto neighbors = aoi.GetNeighbors(id);
    std::cout << "Entity " << id << " neighbors: ";
    for (int n : neighbors) std::cout << n << " ";
    std::cout << std::endl;
}