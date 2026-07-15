#pragma once

#include <iostream>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <thread>
#include <functional>
#include "../Common/ConcurrentQueue.h"


class ClientEntity;
class ClientEntityMgr
{
public:
    ClientEntityMgr(int currentEntityId);
    
    ~ClientEntityMgr();

public:
    void Start();

    void AddNewEntity(int entityId, int x, int y);

    void RemoveEntity(int entityId);

    void MoveEntity(int entityId, int newX, int newY);

    int GetCurrentEntityId() const { return currentEntityId_; }

private:
    void RunUpdateLoop();

    void PushFuncToQueue(std::function<void()> func);

private:
    int currentEntityId_ = 0; // 所属的客户端实体id
    std::unique_ptr<std::thread> updateThread_;
    std::unordered_map<int, std::shared_ptr<ClientEntity>> viewClientEntities_; // 视野内的玩家列表

    moodycamel::ConcurrentQueue<std::function<void()>> funcQueue_;
    std::atomic<bool> stop_flag_ = false;
};