#include "AOIPerformanceTest.h"
#include "IAOIManager.h"
#include "GridAOI.h"
#include "CrossListAOI.h"
#include "QuadTreeAOI.h"


  // 生成随机位置
    std::pair<int, int> AOIPerformanceTest::RandomPosition(std::mt19937& gen, int w, int h) {
        std::uniform_int_distribution<int> distX(0, w - 1);
        std::uniform_int_distribution<int> distY(0, h - 1);
        return {distX(gen), distY(gen)};
    }

    // 运行单个算法的测试
    AOIPerformanceTest::TestResult AOIPerformanceTest::RunTest(IAOIManager& aoi, const AOIPerformanceTest::TestConfig& cfg, std::mt19937& gen) {
        TestResult result;
        result.name = typeid(aoi).name();
        result.insertTimeMs = 0.0;
        result.moveTimeMs = 0.0;
        result.queryTimeMs = 0.0;

        // 1. 插入测试
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<int> entities;
        entities.reserve(cfg.entityCount);
        for (int i = 0; i < cfg.entityCount; ++i) {
            auto [x, y] = RandomPosition(gen, cfg.worldWidth, cfg.worldHeight);
            if (aoi.AddEntity(i, x, y)) {
                entities.push_back(i);
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        result.insertTimeMs = std::chrono::duration<double, std::milli>(end - start).count();

        // 2. 移动测试
        std::uniform_int_distribution<int> pickEntity(0, entities.size() - 1);
        std::uniform_int_distribution<int> delta(-10, 10);
        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < cfg.moveCount; ++i) {
            int idx = pickEntity(gen);
            int entityId = entities[idx];
            auto pos = aoi.GetEntityPosition(entityId);
            if (!pos.valid) continue;
            int newX = pos.x + delta(gen);
            int newY = pos.y + delta(gen);
            // 限制在地图范围内
            newX = std::max(0, std::min(cfg.worldWidth - 1, newX));
            newY = std::max(0, std::min(cfg.worldHeight - 1, newY));
            aoi.MoveEntity(entityId, newX, newY);
        }
        end = std::chrono::high_resolution_clock::now();
        result.moveTimeMs = std::chrono::duration<double, std::milli>(end - start).count();

        // 3. 查询测试
        std::uniform_int_distribution<int> pickQueryEntity(0, entities.size() - 1);
        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < cfg.queryCount; ++i) {
            int idx = pickQueryEntity(gen);
            int entityId = entities[idx];
            auto neighbors = aoi.GetNeighbors(entityId, cfg.radius);
            // 防止编译器优化掉查询结果
            volatile size_t dummy = neighbors.size();
            (void)dummy;
        }
        end = std::chrono::high_resolution_clock::now();
        result.queryTimeMs = std::chrono::duration<double, std::milli>(end - start).count();

        result.totalTimeMs = result.insertTimeMs + result.moveTimeMs + result.queryTimeMs;
        return result;
    }

    // 打印测试结果
    void AOIPerformanceTest::PrintResult(const AOIPerformanceTest::TestResult& result) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Algorithm: " << result.name << std::endl;
        std::cout << "  Insert:  " << result.insertTimeMs << " ms" << std::endl;
        std::cout << "  Move:    " << result.moveTimeMs << " ms" << std::endl;
        std::cout << "  Query:   " << result.queryTimeMs << " ms" << std::endl;
        std::cout << "  Total:   " << result.totalTimeMs << " ms" << std::endl;
        std::cout << std::endl;
    }


    int AOIPerformanceTest::PerformanceTest()
    {
        // 测试配置
        const int ENTITY_COUNT = 5000;
        const int MOVE_COUNT = 10000;
        const int QUERY_COUNT = 5000;
        const int WORLD_SIZE = 10000;
        const int GRID_SIZE = 100;
        const int RADIUS = 1;

        TestConfig cfg;
        cfg.entityCount = ENTITY_COUNT;
        cfg.moveCount = MOVE_COUNT;
        cfg.queryCount = QUERY_COUNT;
        cfg.worldWidth = WORLD_SIZE;
        cfg.worldHeight = WORLD_SIZE;
        cfg.gridSize = GRID_SIZE;
        cfg.radius = RADIUS;

        // 固定随机种子，确保可复现
        std::mt19937 gen(42);

        std::cout << "=== AOI Performance Test ===" << std::endl;
        std::cout << "Entities: " << ENTITY_COUNT << std::endl;
        std::cout << "Moves: " << MOVE_COUNT << std::endl;
        std::cout << "Queries: " << QUERY_COUNT << std::endl;
        std::cout << "World: " << WORLD_SIZE << "x" << WORLD_SIZE << std::endl;
        std::cout << "Grid size: " << GRID_SIZE << std::endl;
        std::cout << "Radius: " << RADIUS << std::endl;
        std::cout << std::endl;

        // 1. 网格法
        {
            GridAOI aoi(cfg.gridSize);
            auto result = RunTest(aoi, cfg, gen);
            PrintResult(result);
        }

        // 重置随机种子以保证不同算法使用相同操作序列
        gen.seed(42);

        // 2. 十字链表
        {
            CrossListAOI aoi(cfg.gridSize);
            auto result = RunTest(aoi, cfg, gen);
            PrintResult(result);
        }

        gen.seed(42);

        // 3. 四叉树
        {
            // 四叉树需要世界大小，且要求为 2 的幂，这里传入 WORLD_SIZE，内部会调整
            QuadTreeAOI aoi(cfg.worldWidth, cfg.worldHeight, cfg.gridSize, 8, 8);
            auto result = RunTest(aoi, cfg, gen);
            PrintResult(result);
        }

        return 0;
    }