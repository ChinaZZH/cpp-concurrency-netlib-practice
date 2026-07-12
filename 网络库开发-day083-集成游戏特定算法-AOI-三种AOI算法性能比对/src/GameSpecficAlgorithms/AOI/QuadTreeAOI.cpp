#include "QuadTreeAOI.h"
#include <iostream>
#include <sstream>
#include "../../Common/LogFile.h"
#include "../../../build/proto_gen/aoi.pb.h"

/*************************************************************************** */
/*******************QuadTreeNode 四叉树的结点逻辑*****************************/
/*************************************************************************** */
 QuadTreeNode::QuadTreeNode(IAOIManager* aoi, const QuadTreeNode::Rect& boundary, int capacity  /*= 0*/
    , int maxDepth /*= 8*/, int depth /*= 0*/)
 : aoi_(aoi)
 , boundary_(boundary)
 ,capacity_(capacity)
 ,maxDepth_(maxDepth)
 ,depth_(depth + 1)
 ,isLeaf_(true)
 ,entityCount_(0)
 {
    for(int i = 0; i < 4; ++i)
    {
        children_[i] = nullptr;
    }
 }
    


bool QuadTreeNode::Insert(int entityId, int x, int y)
{
    // x, y 不属于这个区域
    if(!boundary_.Contains(x, y))
    {
        return false;
    }

    if(isLeaf_)
    {
        // 递归深度已经超过极限 或者当前的实体ID列表还没有到达阈值
        if(depth_ >= maxDepth_ || entityIdList_.size() < capacity_)
        {
            //std::cout << "QuadTreeNode::Insert entityId:=" << entityId << std::endl;
            entityIdList_.push_back(entityId);
            entityCount_ += 1;
            return true;
        }

        // 当前的数值已经大于等于capactiy了，则分裂成4个结点，并把当前的结点都下放
        // 如果无法分割并且不是2的背书，则报错并且返回错误
        if(boundary_.width <= 1 || boundary_.height <= 1 || (boundary_.width & 0x001) > 0 || (boundary_.height & 0x001) > 0)
        {
            std::stringstream ss;
            ss << "GridAOI::AddEntity boundary_ not divide width:=" << boundary_.width << " height:=" << boundary_.height << std::endl; 
            LogFile& logfile = LogFile::getInstance();
            logfile.AppendContent("GridAOI.txt", ss.str());
            return false;
        }

        // std::cout << "QuadTreeNode::DivideToSubChild " << entityId << std::endl;
        this->DivideToSubChild();
        isLeaf_ = false;
    }

    // 处理新的结点插入
    for(int j = 0; j < 4; ++j)
    {
        bool bContain = children_[j]->boundary_.Contains(x, y);
        if(false == bContain)
        {
            continue;
        }
               

        bool bInsertResult = children_[j]->Insert(entityId, x, y);
        if(bInsertResult)
        {
            entityCount_ += 1;
        }

        return bInsertResult;
    }

    return false;
}


void QuadTreeNode::DivideToSubChild()
{
    // 不是叶子结点不能分裂
    if(!isLeaf_)
    {
        std::stringstream ss;
        ss << "QuadTreeNode::DivideToSubChild not leaf node Cannot divide to subChild" << std::endl; 
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("GridAOI.txt", ss.str());
        return;
    }

    

    // 要分裂了就标明他已经不是叶子结点了。
    // 同时数值状态, 将entityIdList_的size也收紧，需要的时候再进行分配内存吧。
    std::vector<int> vecSwapEmpty;
    entityIdList_.swap(vecSwapEmpty);

    int halfWidth  = boundary_.width / 2;
    int halfHeight = boundary_.height / 2;
        
    // 左上
    {
        QuadTreeNode::Rect leftTop(
            boundary_.left_bottom_x, 
            boundary_.left_bottom_y + halfHeight, 
            halfWidth, 
            halfHeight
        );
    
        children_[0] = std::make_unique<QuadTreeNode>(aoi_, leftTop, capacity_, maxDepth_, depth_);
    }
    
    // 右上
    {
        QuadTreeNode::Rect rightTop(
            boundary_.left_bottom_x + halfWidth,
            boundary_.left_bottom_y + halfHeight, 
            halfWidth, 
            halfHeight
        );

        children_[1] = std::make_unique<QuadTreeNode>(aoi_, rightTop, capacity_, maxDepth_, depth_);
    }
    

    // 左下
    {
        QuadTreeNode::Rect leftBottom(
            boundary_.left_bottom_x, 
            boundary_.left_bottom_y, 
            halfWidth, 
            halfHeight
        );

        children_[2] = std::make_unique<QuadTreeNode>(aoi_, leftBottom, capacity_, maxDepth_, depth_);
    }
        

    // 右下
    {
        QuadTreeNode::Rect rightBottom(
            boundary_.left_bottom_x + halfWidth,
            boundary_.left_bottom_y, 
            halfWidth, 
            halfHeight
        );

        children_[3] = std::make_unique<QuadTreeNode>(aoi_, rightBottom, capacity_, maxDepth_, depth_);
    }

       
        
    // 将当前已经有的结点下放
    for(const int& id : vecSwapEmpty)
    {
        auto entityPos = aoi_->GetEntityPosition(id);
        if(!entityPos.valid)
        {
            // 异常，目前做简单处理。只是做日志处理，结点的分裂一次进行下去，只是把这个结点挑拣出来扔掉。
            // 打印这个日志，说明上层的调用层业务出错了。
            std::stringstream ss;
            ss << "QuadTreeNode::AddEntity aoi Get Entity Positon error entityId:=" << id << std::endl; 
            LogFile& logfile = LogFile::getInstance();
            logfile.AppendContent("QuadTreeNode.txt", ss.str());
            continue;
        }


        for(int j = 0; j < 4; ++j)
        {
            bool bContain = children_[j]->boundary_.Contains(entityPos.x, entityPos.y);
            if(false == bContain)
            {
                // std::cout << "QuadTreeNode::DivideToSubChild Contains false index:=" << j << " id:=" << id << std::endl;
                continue;
            }
               

            // std::cout << "QuadTreeNode::DivideToSubChild Contains true index:=" << j << " id:=" << id << std::endl;
            children_[j]-> Insert(id, entityPos.x, entityPos.y);
            break;
        }
    }
       
   
}


bool QuadTreeNode::Remove(int entityId)
{
    auto entityPos = aoi_->GetEntityPosition(entityId);
    if (!entityPos.valid)
    {
        std::stringstream ss;
        ss << "GridAOI::Remove aoi Get Entity Positon error entityId:=" << entityId << std::endl; 
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("GridAOI.txt", ss.str());
        return false;
    }

    if(false == boundary_.Contains(entityPos.x, entityPos.y))
    {
        std::stringstream ss;
        ss << "GridAOI::Remove entity Positon error position(x:=" << entityPos.x << " , y:=" << entityPos.y << " )" << std::endl; 
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("GridAOI.txt", ss.str());
        return false;
    }

    // 叶子结点，则直接移除
    if(isLeaf_)
    {
        auto itr = std::remove(entityIdList_.begin(), entityIdList_.end(), entityId);
        if (itr != entityIdList_.end())
        {
            entityIdList_.erase(itr, entityIdList_.end());
            entityCount_ -= 1;
            return true;
        }
        
        // 叶子结点但是没有找到这个结点
        std::stringstream ss;
        ss << "GridAOI::Remove entity not fuond entityId:=" << entityId << std::endl; 
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("GridAOI.txt", ss.str());
        return false;
    }

    // 非叶子结点，则递归的去删除该节点
    bool removeFlag = false;
    for(int j = 0; j < 4; ++j)
    {
        bool bContain = children_[j]->boundary_.Contains(entityPos.x, entityPos.y);
        if(!bContain)
        {
            continue;
        }
               

        removeFlag = children_[j]-> Remove(entityId);
        if(false == removeFlag)
        {
            std::stringstream ss;
            ss << "GridAOI::Remove remove error entityId:=" << entityId << std::endl; 
            LogFile& logfile = LogFile::getInstance();
            logfile.AppendContent("GridAOI.txt", ss.str());
            return false;
        }

        removeFlag = true;
        entityCount_ -= 1;
        break;
    }

    if(false == removeFlag)
    {
        std::stringstream ss;
        ss << "GridAOI::Remove entity Positon ServerLogic Error position(x:=" << entityPos.x << " , y:=" << entityPos.y << " )" << std::endl; 
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("GridAOI.txt", ss.str());
        return false;
    }

    // 已经删除则统计下最新的结点个数, 如果依旧大于capacity则保持分裂。
   if(this->GetTotalEntityCount() >= capacity_)
   {
        return true;
   }

   // 否则合并
   entityIdList_.clear();
   Merge(entityIdList_);
   isLeaf_ = true;
   entityCount_ = entityIdList_.size();
   return true; 
}

bool QuadTreeNode::Merge(std::vector<int>& result)
{
    if(isLeaf_)
    {
        std::copy(entityIdList_.begin(), entityIdList_.end(), std::back_inserter(result));
        return true;
    }

    for(int i = 0; i < 4; ++i)
    {
        children_[i]->Merge(result);
        children_[i].reset();
    }

    isLeaf_ = true;
    return true;
}



bool QuadTreeNode::Update(int entityId, int newX, int newY)
{
    auto entityPos = aoi_->GetEntityPosition(entityId);
    if (!entityPos.valid)
    {
        std::stringstream ss;
        ss << "GridAOI::Update aoi Get Entity Positon error entityId:=" << entityId << std::endl; 
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("GridAOI.txt", ss.str());
        return false;
    }

    // 和上次保存的位置相同，则无需更改
    if(entityPos.x == newX && entityPos.y == newY)
    {
        return true;
    }

    if(false == boundary_.Contains(entityPos.x, entityPos.y))
    {
        std::stringstream ss;
        ss << "GridAOI::Update entity Positon error position(x:=" << entityPos.x << " , y:=" << entityPos.y << " )" << std::endl; 
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("GridAOI.txt", ss.str());
        return false;
    }

    // 需要判断是否是在同一个叶子叶子结点内，如果是的话只要修改数值即可。如果不是的话则需要先删除在新增。
    // 递归判断是否和entityId在同一个叶子结点内
    bool bCheck = this->CheckOnSameNodeLeaf(entityId, newX, newY);
    if(bCheck)
    {
        return true;
    }

    // 先删除
    this->Remove(entityId);
    this->Insert(entityId, newX, newY);
    return true;
}
    

bool QuadTreeNode::CheckOnSameNodeLeaf(int entityId, int newX, int newY)
{
    if (this->isLeaf_)
    {
        // 没找到
        auto itr = std::find(entityIdList_.begin(), entityIdList_.end(), entityId);
        if(itr == entityIdList_.end())
        {
            return false;
        }

        // 找到了，判断newX,newY 是否在区域内
        return boundary_.Contains(newX, newY);
    }


    // 非叶子结点，则顺着杆继续往下找
    for(int i = 0; i < 4; i++)
    {
        bool bCheck = children_[i]->CheckOnSameNodeLeaf(entityId, newX, newY);
        if (bCheck)
        {
            return true;
        }
    }

    return false;
}


void QuadTreeNode::Query(const Rect& range, std::set<int>& result) const
{
    // 两块区域没有重合区域
    if(!boundary_.Intersets(range))
    {
        return ;
    }

    if(isLeaf_)
    {
        for(int id : entityIdList_)
        {
            auto entityPos = aoi_->GetEntityPosition(id);
            if (!entityPos.valid)
            {
                std::stringstream ss;
                ss << "GridAOI::Query aoi Get Entity Positon error entityId:=" << id << std::endl; 
                
                LogFile& logfile = LogFile::getInstance();
                logfile.AppendContent("GridAOI.txt", ss.str());
                continue;
            }

            // std::cout << "QuadTreeNode::Query entity_id:=" << id << std::endl;
            if(false == range.Contains(entityPos.x, entityPos.y))
            {
                continue;
            }

            // std::cout << "QuadTreeNode::Query insert node entity_id:=" << id << std::endl;
            result.insert(id);
        }
    }
    else
    {
         // 非叶子结点，则顺着杆继续往下找
        for(int i = 0; i < 4; i++)
        {
            bool bIntersets = children_[i]->boundary_.Intersets(range);
            if (false == bIntersets)
            {
                continue;
            }

            children_[i]->Query(range, result);
        }
    }
}



/*************************************************************************** */
/*******************QuadTreeAOI 四叉树的AOI算法实现*****************************/
/*************************************************************************** */

QuadTreeAOI::QuadTreeAOI(int worldWidth, int worldHeight, int gridSize, int nodeCapacity, int maxDepth)
    :BaseAOIManager(gridSize)
{
    // 必须是偶数，奇数则报错
    if((gridSize & 0x001) > 0)
    {
        exit(1);
    }

    // 将worldWidth转化成2的幂次方
    {
        int depth = 0;
        int transformValue = 1;
        while(transformValue < worldWidth && depth < maxDepth)
        {
            transformValue = transformValue << 1;
            depth += 1;
        }

        worldWidth = transformValue;
    }
    

    // 将worldHeight转化成2的幂次方
    {
        int depth = 0;
        int transformValue = 1;
        while(transformValue < worldHeight && depth < maxDepth)
        {
            transformValue = transformValue << 1;
            depth += 1;
        }
        
        worldHeight = transformValue;
    }

    QuadTreeNode::Rect rect(0, 0, worldWidth, worldHeight);
    root_ = std::make_unique<QuadTreeNode>(this, rect, nodeCapacity, maxDepth, 0);
}

 QuadTreeAOI::~QuadTreeAOI() 
 { 
    root_.reset();
 }

bool QuadTreeAOI::AddEntity(int entityId, int x, int y)
{
    auto itr = entityMap_.find(entityId);
    if(itr != entityMap_.end())
    {
        std::stringstream ss;
        ss << "QuadTreeAOI::AddEntity error entityID:=" << entityId << std::endl; 
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("QuadTreeAOI.txt", ss.str());

        std::cout << "QuadTreeAOI::AddEntity error entityID:=" << entityId << std::endl; 
        return false;
    }

    if(false == root_->Insert(entityId, x, y))
    {
        return false;
    }

    // 存储数据修改
    EntityInfo entity;
    entity.x = x;
    entity.y = y;
    auto gridPostion = this->GetGridCoord(x, y);
    entity.gridX = gridPostion.first;
    entity.gridY = gridPostion.second;
    entityMap_[entityId] = entity;
    
    // 广播有新entity消息下发newEntityResponse，发送entityId, x, y
    if(msgNotifyer_)
    {
        auto neighborsEntityList = this->GetNeighbors(entityId);
        msgNotifyer_->EnterNewGridPosMsgNotify(entityId, x, y, neighborsEntityList);
    }
    
    return true;
}


bool QuadTreeAOI::RemoveEntity(int entityId)
{
    auto itrEntity = entityMap_.find(entityId);
    if(itrEntity == entityMap_.end())
    {
        std::stringstream ss;
        ss << "QuadTreeAOI::RemoveEntity error entityID:=" << entityId << std::endl;
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("QuadTreeAOI.txt", ss.str());

        std::cout << "QuadTreeAOI::RemoveEntity error entityID:=" << entityId << std::endl; 
        return false;
    }

    if(false == root_->Remove(entityId))
    {
        return false;
    }

    std::vector<int> neighborsEntityList;
    if(msgNotifyer_)
    {
       neighborsEntityList = this->GetNeighbors(entityId);
    }

    // 存储数据修改
    {
        const EntityInfo& entity = (itrEntity->second);
        std::pair<int, int> gridPostion(entity.gridX, entity.gridY);
        entityMap_.erase(itrEntity);
    }
    
    if(msgNotifyer_)
    {
        msgNotifyer_->LeaveGridToMsgNotify(entityId, neighborsEntityList);
    }

    return true;
}


bool QuadTreeAOI::MoveEntity(int entityId, int newX, int newY)
{
    // 移动entity的前提是 原先entityId就已经在了。
    auto itrEntity = entityMap_.find(entityId);
    if(itrEntity == entityMap_.end())
    {
        std::stringstream ss;
        ss << "QuadTreeAOI::MoveEntity error entityID:=" << entityId << std::endl;
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("QuadTreeAOI.txt", ss.str());

        std::cout << "QuadTreeAOI::MoveEntity error entityID:=" << entityId << std::endl; 
        return false;
    }

    // 位置没有发生变化的时候，直接返回
    EntityInfo& entityInfo = (itrEntity->second);
    if(entityInfo.x == newX && entityInfo.y == newY)
    {
        return true;
    }

    std::vector<int>  oldNeighborsList;
    if(msgNotifyer_)
    {
        oldNeighborsList  = this->GetNeighbors(entityId);
    }
   
    if(false == root_->Update(entityId, newX, newY))
    {
        return false;
    }


    // 更新entityInfo
    auto newGridPos = this->GetGridCoord(newX, newY);
    entityInfo.x = newX;
    entityInfo.y = newY;
    entityInfo.gridX = newGridPos.first;
    entityInfo.gridY = newGridPos.second;

    //同步数据
    if(msgNotifyer_)
    {
        auto newNeighborsEntitys = this->GetNeighbors(entityId);
        msgNotifyer_->MovePositionToMsgNotifyForGridChange(entityId, newX, newY, oldNeighborsList, newNeighborsEntitys);
    }

    return true;
}

// 获取周围实体列表（默认九宫格，radius 可扩展）
std::vector<int> QuadTreeAOI::GetNeighbors(int entityId, int radius /*= 1*/) const
{
    auto itrEntity = entityMap_.find(entityId);
    if(itrEntity == entityMap_.end())
    {
        return std::vector<int>();
    }

    int height = gridSize_ * 2 + gridSize_;
    int width = height;

    const EntityInfo& entity = (itrEntity->second);
    int rectX = entity.x -  height / 2;
    int rectY = entity.y -  width / 2;

    // std::cout << "QuadTreeAOI::GetNeighbors rect left_bottom_position:(x," << rectX << "y" << rectY << " ) hegiht:=" << height << " width:=" << width << std::endl;  
    QuadTreeNode::Rect rect(rectX, rectY, width, height);
    
    std::set<int> setNeighbors;
    root_->Query(rect, setNeighbors);
    setNeighbors.erase(entityId);

    std::vector<int> vecNeighbors;
    std::copy(setNeighbors.begin(), setNeighbors.end(), std::back_inserter(vecNeighbors));
    return vecNeighbors;
}

// 辅助，根据实体坐标转换为网格坐标
GridCoordResult QuadTreeAOI::GetGridPosition(int entityId) const
{
    GridCoordResult result;
    result.valid = false;
    result.gridX = 0;
    result.gridY = 0;
    auto itr = entityMap_.find(entityId);
    if(itr == entityMap_.end()) 
    {
        return result;
    }

    const EntityInfo& entity = (itr->second);
    result.valid = true;
    result.gridX = entity.gridX;
    result.gridY = entity.gridY;
    return result;
}

EntityPositionResult QuadTreeAOI::GetEntityPosition(int entityId) const
{
     EntityPositionResult result;
    result.valid = false;
    result.x = 0;
    result.y = 0;
    auto itr = entityMap_.find(entityId);
    if(itr == entityMap_.end()) 
    {
        return result;
    }

    const EntityInfo& entity = (itr->second);
    result.valid = true;
    result.x = entity.x;
    result.y = entity.y;
    return result;
}