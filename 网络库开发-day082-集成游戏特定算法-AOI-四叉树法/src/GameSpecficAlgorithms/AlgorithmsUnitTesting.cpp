#include "AlgorithmsUnitTesting.h"
#include "AOI/GridAOI.h"
#include "AOI/CrossListAOI.h"
#include "AOI/QuadTreeAOI.h"

 AlgorithmsUnitTesting::AlgorithmsUnitTesting()
 {

 }

 // 测试aoi算法九宫格
void AlgorithmsUnitTesting::TestAoiManager()
{
    std::shared_ptr<IAOIManager> aoi = std::make_shared<CrossListAOI>(100);
    //CrossListAOI aoi(100);

    aoi->AddEntity(1, 50, 50);
    std::cout << "Helle world" << std::endl;
    aoi->AddEntity(2, 120, 50);
    aoi->AddEntity(3, 50, 120);
    aoi->AddEntity(4, 200, 200);    
    PrintNeighbors(aoi, 1); // 应该在九宫格内看到 2, 3

    // 移动实体
    aoi->MoveEntity(2, 100, 50);
    PrintNeighbors(aoi, 1); // 2 可能仍在九宫格内

    aoi->MoveEntity(2, 300, 300);
    PrintNeighbors(aoi, 1); // 2 应该离开九宫格

    // 删除实体
    aoi->RemoveEntity(3);
    PrintNeighbors(aoi, 1);
}


void AlgorithmsUnitTesting::TestQuadTreeAOI() {
    std::shared_ptr<IAOIManager> aoi = std::make_shared<QuadTreeAOI>(1000, 1000, 100, 2, 8); // 世界 1000x1000，网格 100
    aoi->AddEntity(1, 50, 50);
    std::cout << "AddEntity  1 " << std::endl;
    aoi->AddEntity(2, 120, 50);
    std::cout << "AddEntity  2 " << std::endl;
    aoi->AddEntity(3, 50, 120);
    std::cout << "AddEntity 3 " << std::endl;
    aoi->AddEntity(4, 200, 200);
    std::cout << "AddEntity 4 " << std::endl;

    auto neighbors = aoi->GetNeighbors(1);
    std::cout << "QuadTree neighbors of 1: ";
    for (int id : neighbors) std::cout << id << " ";
    std::cout << std::endl;

    
    aoi->MoveEntity(2, 300, 300);
    neighbors = aoi->GetNeighbors(1);
    std::cout << "After move, neighbors of 1: ";
    for (int id : neighbors) std::cout << id << " ";

    std::cout << std::endl;
    
    aoi->RemoveEntity(3);
    neighbors = aoi->GetNeighbors(1);
    std::cout << "After remove, neighbors of 1: ";
    for (int id : neighbors) std::cout << id << " ";
    std::cout << std::endl;
}

void AlgorithmsUnitTesting::PrintNeighbors(std::shared_ptr<IAOIManager> aoi, int id)
{
    auto neighbors = aoi->GetNeighbors(id);
    std::cout << "Entity " << id << " neighbors: ";
    for (int n : neighbors) std::cout << n << " ";
    std::cout << std::endl;
}