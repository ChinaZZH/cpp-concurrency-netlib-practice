#pragma once


class Test_A_Star
{
public:
    void TestDataStruct_1();

    void Test_PathFinder();

    void Test_PathSmoother();

private:
    void TestGridMap();
    
    void TestNodeManager();

private:
    void TestBasicPath();
    void TestWithObstacle();
    void TestDiagonalPath();
    void TestUnreachable();

private:
    void TestHelpPathSmoothing();
    void TestLineOfSight();
};



