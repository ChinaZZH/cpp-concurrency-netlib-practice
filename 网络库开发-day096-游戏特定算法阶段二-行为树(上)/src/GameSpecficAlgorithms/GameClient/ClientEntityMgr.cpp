#include "ClientEntityMgr.h"
#include "ClientEntity.h"

ClientEntityMgr::ClientEntityMgr(int currentEntityId)
    :currentEntityId_(currentEntityId)
    ,updateThread_(nullptr)
{

}


ClientEntityMgr::~ClientEntityMgr()
{
    {
        stop_flag_.store(false, std::memory_order_release);
    }

    if(updateThread_ && updateThread_->joinable())
    {
        updateThread_->join();
    }
}


void ClientEntityMgr::Start()
{
    updateThread_ = std::make_unique<std::thread>([this](){ 
        while(!stop_flag_.load(std::memory_order_acquire))
        {
            this->RunUpdateLoop();
        }
    });
}

void ClientEntityMgr::AddNewEntity(int entityId, int x, int y)
{
    if(entityId == currentEntityId_)
    {
        return;
    }

    // 投递到 updateThread_执行
    this->PushFuncToQueue([entityId, x, y, this](){
        // 视野内已经有该实体，则返回
        auto itr = viewClientEntities_.find(entityId);
        if(itr != viewClientEntities_.end())
        {
            return;
        }

        std::shared_ptr<ClientEntity> newEntity = std::make_shared<ClientEntity>(entityId, x, y);
        viewClientEntities_.emplace(entityId, newEntity);
    });
}

void ClientEntityMgr::RemoveEntity(int entityId)
{
    if(entityId == currentEntityId_)
    {
        return;
    }

    // 投递到 updateThread_执行
    this->PushFuncToQueue([entityId, this](){
        viewClientEntities_.erase(entityId);
    });
}

void ClientEntityMgr::MoveEntity(int entityId, int newX, int newY)
{
    if(entityId == currentEntityId_)
    {
        return;
    }

    // 投递到 updateThread_执行
    this->PushFuncToQueue([entityId, newX, newY, this](){
        // 视野内没有该实体，则返回
        auto itr = viewClientEntities_.find(entityId);
        if(itr == viewClientEntities_.end())
        {
            return;
        }

        std::shared_ptr<ClientEntity> entity = itr->second;
        entity->OnMoveAction(newX, newY);
    });
}

void ClientEntityMgr::PushFuncToQueue(std::function<void()> func)
{
    funcQueue_.enqueue(func);
}

void ClientEntityMgr::RunUpdateLoop()
{
    //std::cout << "start ClientEntityMgr::RunUpdateLoop" << std::endl;
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    std::function<void()> funcTask;
    while(funcQueue_.try_dequeue(funcTask))
    {
        if(funcTask)
        {
            funcTask();
        }
    }
        
    //std::cout << "startSleep ClientEntityMgr::RunUpdateLoop" << std::endl;
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    auto elapseSec = std::chrono::duration<float>(now - startTime).count();
    if(elapseSec < 0.05f) // 每50ms更新一次
    {
        int millSec = static_cast<int>((0.05f - elapseSec) * 1000);
        std::this_thread::sleep_for(std::chrono::milliseconds(millSec));  // 不够50ms则睡眠一段时间
    }

    //std::cout << "startUpdateLoop ClientEntityMgr::RunUpdateLoop" << std::endl;
    for(auto& [id, entity] : viewClientEntities_)
    {
        entity->UpdateLoop();
    }
    //std::cout << "startUpdateLoop ClientEntityMgr::RunUpdateLoop" << std::endl;
}