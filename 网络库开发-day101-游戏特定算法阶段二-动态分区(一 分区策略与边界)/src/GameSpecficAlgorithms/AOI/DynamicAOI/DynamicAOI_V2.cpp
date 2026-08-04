#include "DynamicAOI_V2.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <cstdlib> 
#include "../GridAOI.h"
#include "../CrossListAOI.h"
#include "../QuadTreeAOI.h"

// ----- ----- ----------------------------------------------------------------------------------------------
// ----- -----------------------------------RegionNode_V2 开始 ---------------------------------------------
// ----- ----- ----------------------------------------------------------------------------------------------

void RegionNode_V2::AddPlayer(int player_id, int newX, int newY)
{
    stats.player_count += 1;
    storage->AddEntity(player_id, newX, newY);
}

void RegionNode_V2::RemovePlayer(int player_id)
{
    stats.player_count -= 1;
    if(storage)
    {
        storage->RemoveEntity(player_id);
    }
    else
    {
        std::cout << "RegionNode_V2::RemovePlayer player_id:=" << player_id << " region_id:=" << stats.region_id << std::endl;
    }
}

void RegionNode_V2::ClearPlayer()
{
    stats.player_count = 0;
}

void RegionNode_V2::ClearStorage()
{
    //std::cout << "RegionNode_V2::ClearStorage region_id:=" << stats.region_id << std::endl;
    stats.player_count = 0;
    storage.reset();
}

void RegionNode_V2::CreateStorage(int aoi_type, int grid_size)
{
    if(0 == aoi_type)
    {
        storage = std::make_unique<GridAOI>(grid_size);
    }
    else if(1 == aoi_type)
    {
        storage = std::make_unique<CrossListAOI>(grid_size);
    }
    else if(2 == aoi_type)
    {
        int width = bounds.GetWidth();
        int height = bounds.GetHeight();
        //std::cout << "RegionNode_V2::CreateStorage width:=" << width << " height:=" << height << std::endl;
        //std::cout << "RegionNode_V2::CreateStorage region_id:=" << stats.region_id << " min_x:=" << bounds.min_x << " min_y:="  << bounds.min_y << std::endl;
        storage = std::make_unique<QuadTreeAOI>(width, height, grid_size, 0, 0, bounds.min_x, bounds.min_y);
    }
    else
    {
        std::cout << "RegionNode_V2::CreateStorage region_id:=" << stats.region_id << " error aoi_type:=" << aoi_type << std::endl;
    }
}

int RegionNode_V2::PlayerCount() const
{
    return stats.player_count;
}

// ----- ----- ----------------------------------------------------------------------------------------------
// ----- -----------------------------------DynamicAOI_V2 开始 ---------------------------------------------
// ----- ----- ----------------------------------------------------------------------------------------------

DynamicAOI_V2::DynamicAOI_V2(int aoi_type, int max_depth /*= 0*/, int grid_size /*= 100*/, int worldWidth /*= 1024*/, int worldHeight /*= 1024*/)
: aoi_type_(aoi_type)
, max_depth_(max_depth)
, grid_size_(grid_size)
{
    // aoi_type_ 必须大于2
    if(aoi_type_ > 2)
    {
        exit(1);
    }

    // 必须是偶数，奇数则报错
    if((grid_size & 0x001) > 0)
    {
        exit(1);
    }

    // 将worldWidth转化成2的幂次方
    {
        int transformValue = 1;
        while(transformValue < worldWidth)
        {
            transformValue = transformValue << 1;
        }

        worldWidth = transformValue;
    }
    

    // 将worldHeight转化成2的幂次方
    {
        int transformValue = 1;
        while(transformValue < worldHeight)
        {
            transformValue = transformValue << 1;
        }
        
        worldHeight = transformValue;
    }

    //std::cout << "DynamicAOI_V2::DynamicAOI_V2 wroldWidth:=" << worldWidth << " worldHeight:=" << worldHeight << std::endl;
    root_region_id_ = this->AllocateRegionId();
    RegionNode_V2 node;
    node.parent_id = 0;
    node.bounds = AABB{0, 0, worldWidth, worldHeight};
    node.depth = 1;

    node.stats.region_id = root_region_id_;
    node.stats.area_size = node.bounds.GetArea();                //  区域面积（用于计算密度）
    node.stats.last_split_frame = 0;                             //  上次分裂帧号（防抖动）
    node.stats.last_merge_frame = 0;                             //  上次合并帧号（防抖动）

    node.ClearPlayer();   //  当前区域玩家数
    node.CreateStorage(aoi_type_, grid_size_);
    
    

    regions_[root_region_id_] = (std::move(node));
}

// ----- ----- ----------------------------------------------------------------------------------------------
// ----- -----------------------------------IAOIManager 接口实现 ---------------------------------------------
// ----- ----- ----------------------------------------------------------------------------------------------
bool DynamicAOI_V2::AddEntity(int entity_id, int x, int y) 
{
    auto itr = entity_to_region_.find(entity_id);
    if(itr != entity_to_region_.end())
    {
        return MoveEntity(entity_id, x, y);
    }

    uint32_t target_region_id = FindLeafRegion(root_region_id_, x, y);
    MoveEntityToRegion(entity_id, target_region_id, x, y);
    CheckAndAdjust(target_region_id);
    return true;
}


bool DynamicAOI_V2::RemoveEntity(int entity_id) 
{
    auto itr = entity_to_region_.find(entity_id);
    if(itr == entity_to_region_.end())
    {
        return false;
    }

    uint32_t target_region_id = (itr->second).region_id;
    RegionNode_V2* region_node = this->GetRegion(target_region_id);
    if(nullptr == region_node)
    {
        return false;
    }
    
    region_node->RemovePlayer(entity_id);

    entity_to_region_.erase(entity_id);
    //std::cout << "DynamicAOI_V2::RemoveEntity player_count:=" << (region_node->PlayerCount()) << std::endl;
    CheckAndAdjust(target_region_id);
    return true;
}

bool DynamicAOI_V2::MoveEntity(int entity_id, int newX, int newY) 
{
    auto itr = entity_to_region_.find(entity_id);
    if(itr == entity_to_region_.end())
    {
        return AddEntity(entity_id, newX, newY);
    }
    
    // 坐标相同，则无需调整
    DynamicAOI_V2::EntityRegionInfo& entity_info = (itr->second);
    if(entity_info.pos.first == newX && entity_info.pos.second == newY)
    {
        return true;
    }

    // 在同一个区域内，则更新坐标即可
    uint32_t old_region_id = entity_info.region_id;
    uint32_t new_region_id = FindLeafRegion(root_region_id_, newX, newY);
    if(old_region_id == new_region_id)
    {
        entity_info.pos.first = newX;
        entity_info.pos.second = newY;

        auto& old_region_node = regions_[old_region_id];
        old_region_node.storage->MoveEntity(entity_id, newX, newY);
        return true;
    }

    MoveEntityToRegion(entity_id, new_region_id, newX, newY);
    CheckAndAdjust(old_region_id);
    CheckAndAdjust(new_region_id);
    return true;
}

std::vector<int> DynamicAOI_V2::GetNeighbors(int entity_id, int radius /*= 1*/) const 
{
    auto itr = entity_to_region_.find(entity_id);
    if(itr == entity_to_region_.end())
    {
        return std::vector<int>();
    }

    // 递归， 自顶向下搜索符合条件的
    int position_radius_length =  (radius * grid_size_ + grid_size_ / 2);

    auto entity_pos = (itr->second).pos;
    AABB neighborAABB;
    neighborAABB.min_x = entity_pos.first - position_radius_length;
    neighborAABB.max_x = entity_pos.first + position_radius_length;
    neighborAABB.min_y = entity_pos.second - position_radius_length;
    neighborAABB.max_y = entity_pos.second + position_radius_length;

    // 判断是否只在一个区域便能完成, 则有限单区域查找
    do
    {
        auto itr_region = regions_.find((itr->second).region_id);
        if(itr_region == regions_.end())
        {
            break;
        }

        const RegionNode_V2& entity_region = (itr_region->second);
        if(false == entity_region.bounds.IsContain(neighborAABB))
        {
            break;
        }


       std::vector<int> vecResult = entity_region.storage->GetNeighbors(entity_id, radius);
       return vecResult;
    }
    while(0);

    // 玩家个数小于阈值的话直接遍历
    std::vector<int> vecNeighborsId;
    if(entity_to_region_.size() <= 100) // 玩家个数小于阈值的话直接遍历
    {
        for(const auto& [neighbor_id, info] : entity_to_region_)
        {
            if(neighbor_id == entity_id)
            {
                continue;
            }

            if(false == neighborAABB.Contains(info.pos.first, info.pos.second))
            {
                continue;
            }

            vecNeighborsId.push_back(neighbor_id);
        }
    }
    else
    {
        // 否则使用递归算法找到合适区域来获取玩家
        std::vector<int> vecResult = FindPosInAABB(root_region_id_, neighborAABB);
        vecNeighborsId = std::move(vecResult);
        std::erase(vecNeighborsId, entity_id);
    }

    return vecNeighborsId;
}



std::vector<int> DynamicAOI_V2::Query(int queryX, int queryY, int queryLength)
{
    AABB neighborAABB;
    neighborAABB.min_x = queryX - queryLength;
    neighborAABB.max_x = queryX + queryLength;
    neighborAABB.min_y = queryY - queryLength;
    neighborAABB.max_y = queryY + queryLength;

    // 玩家个数小于阈值的话直接遍历
    std::vector<int> vecNeighborsId;
    if(entity_to_region_.size() <= 100) // 玩家个数小于阈值的话直接遍历
    {
        for(const auto& [entity_id, info] : entity_to_region_)
        {
            if(false == neighborAABB.Contains(info.pos.first, info.pos.second))
            {
                continue;
            }

            vecNeighborsId.push_back(entity_id);
        }
    }

    else
    {
        // 否则使用递归算法找到合适区域来获取玩家
        std::vector<int> vecResult = FindPosInAABB(root_region_id_, neighborAABB);
        vecNeighborsId = std::move(vecResult);
    }

    return vecNeighborsId;
}


EntityPositionResult DynamicAOI_V2::GetEntityPosition(int entity_id) const 
{
    auto itr = entity_to_region_.find(entity_id);
    if(itr == entity_to_region_.end())
    {
        return EntityPositionResult();
    }

    EntityPositionResult result;
    result.valid = true;
    result.x = (itr->second).pos.first;
    result.y = (itr->second).pos.second;
    return result;
}

std::vector<int> DynamicAOI_V2::FindPosInAABB(uint32_t region_id, const AABB& neighborAABB) const
{
    std::vector<int> vecNeighborsId;
    auto itr = regions_.find(region_id);
    if(itr == regions_.end())
    {
        return vecNeighborsId;
    }

    // 判断两者有没有重合的区域
    const RegionNode_V2& node = (itr->second);
    if(false == node.bounds.IsIntersectWith(neighborAABB))
    {
        return vecNeighborsId;
    }

    // 有重合区域，则判断是否是叶子结点，不是的话，则把叶子结点返回的内容合并
    if(false == node.children.empty())
    {
        // 优化，首个进行std::move， 防止只有单个区域有效的话一直拷贝构造
        bool firstTag = true;
        for(const auto& child_region_id : node.children)
        {
            std::vector<int> vecChildResult = this->FindPosInAABB(child_region_id, neighborAABB);
            if(vecChildResult.empty())
            {
                continue;
            }

            if(firstTag)
            {
                vecNeighborsId = std::move(vecChildResult);
                firstTag = false;
            }
            else
            {
                 std:copy(vecChildResult.begin(), vecChildResult.end(), std::back_inserter(vecNeighborsId));
            }
           
        }
        
        return vecNeighborsId;
    }

    // 叶子结点，则判断每一个entity是否在 neighborAABB 矩形内
    std::vector<BaseEntityData> vecEntityData = node.storage->GetAllEntities();
    for(const auto& base_data : vecEntityData)
    {
        auto itr_entity = entity_to_region_.find(base_data.id);
        if(itr_entity == entity_to_region_.end())
        {
            continue;
        }

        const DynamicAOI_V2::EntityRegionInfo& entity_info = (itr_entity->second);
        if(false == neighborAABB.Contains(entity_info.pos.first, entity_info.pos.second))
        {
            continue;
        }

        vecNeighborsId.push_back(base_data.id);
    }

    return vecNeighborsId;
}


// ----- ----- ----------------------------------------------------------------------------------------------
// ----- -----------------------------------IDynamicAOI 接口实现---------------------------------------------
// ----- ----- ----------------------------------------------------------------------------------------------

//  获取指定区域的玩家密度 (玩家数/区域面积)
float DynamicAOI_V2::GetDensity(uint32_t region_id) const 
{
    auto itr = regions_.find(region_id); 
    if(itr == regions_.end())
    {
        return 0.00f;
    }

    const RegionNode_V2& node = (itr->second);
    //std::cout << "DynamicAOI_V2::GetDensity player_count:=" <<  node.PlayerCount() << " area_size:=" << node.stats.area_size << std::endl;
    return static_cast<float>(node.PlayerCount()) * 1.0000f / static_cast<float>(node.stats.area_size * 1.000f);
}


// 获取当前所有区域的 分裂状态/合并状态 （用于调试/日志）
std::vector<RegionInfo> DynamicAOI_V2::GetRegionInfos() const 
{
    std::vector<RegionInfo> result;
    result.reserve(regions_.size());

    for(const auto& [region_id, region_node] : regions_)
    {
        RegionInfo info;
        info.region_id = region_id;
        info.player_count = region_node.PlayerCount();
        info.area_size = region_node.stats.area_size;
        info.state = this->CalcuRegionState(region_node);
        info.children = region_node.children;
        info.parent_id = region_node.parent_id;

        result.emplace_back(info);
    }


    return result;
}


RegionState DynamicAOI_V2::CalcuRegionState(const RegionNode_V2& node) const
{
    if(0 == node.PlayerCount()) {
        return RegionState::Empty;
    }

    // 分裂冷却中：稳定（不会触发任何动作）
    uint32_t frame = GetCurrentFrame();
    if(frame - node.stats.last_split_frame < split_cooldown_frames_) {
        return RegionState::Stable;
    }

    // 分裂冷却中：稳定（不会触发任何动作）
    if(frame - node.stats.last_merge_frame < merge_cooldown_frames_) {
        return RegionState::Stable;
    } 
    
    float fDensity = static_cast<float>(node.PlayerCount())*1.00000f / node.stats.area_size;
    if (fDensity > split_threshold_) {
        return RegionState::Splitting;
    } 
        
    if(fDensity < merge_threshold_) {
         return RegionState::Merging;
    } 
        
        
    return RegionState::Stable;
}

// 手动触发一次区域重划分(用于测试或者主动调优)
void DynamicAOI_V2::Rebalance() 
{
    if(entity_to_region_.empty() && regions_.size() <= 1)
    {
        return;
    }

    std::vector<uint32_t> vecRegions;
    for(const auto& kv : regions_) {
        vecRegions.push_back(kv.first);
    }

    for(const auto& region_id : vecRegions) {
        auto itr = regions_.find(region_id);
        if(itr == regions_.end())
        {
            continue;
        }

        CheckAndAdjust(region_id);
    }
}

// ----- 核心访问接口（返回指针） -----
RegionNode_V2* DynamicAOI_V2::GetRegion(uint32_t region_id)
{
    auto itr = regions_.find(region_id);
    if(itr == regions_.end())
    {
        return nullptr;
    }

    return &(itr->second);
}

const RegionNode_V2* DynamicAOI_V2::GetRegion(uint32_t region_id) const
{
    auto itr = regions_.find(region_id);
    if(itr == regions_.end())
    {
        return nullptr;
    }

    return &(itr->second);
}


// ----- ----- ----------------------------------------------------------------------------------------------
// ----- -----------------------------------内部核心方法---------------------------------------------
// ----- ----- ----------------------------------------------------------------------------------------------

void DynamicAOI_V2::SplitRegion(uint32_t region_id)
{
    auto itr = regions_.find(region_id);
    if(itr == regions_.end())
    {
        return;
    }

    // 已经分裂过了，无法再分裂
    RegionNode_V2& region_node = (itr->second);
    if(false == region_node.children.empty())
    {
        return;
    }

    // 已经到达整数分裂的极限，不能再分裂了，
    {
        if(2 == aoi_type_ && region_node.depth > 8)
        {
            return;
        }

        int length_x = region_node.bounds.max_x - region_node.bounds.min_x;
        int length_y = region_node.bounds.max_y - region_node.bounds.min_y; 
        if(length_x <= grid_size_ || length_y <= grid_size_)
        {
            return;
        }
    }

    std::vector<uint32_t> split_region_id;
    int old_regions_num = regions_.size();
    std::vector<AABB> vecChild_AABB = region_node.bounds.Split();
    for(int i = 0; i < vecChild_AABB.size(); ++i)
    {
        uint32_t child_region_id = this->AllocateRegionId();
        RegionNode_V2& node = regions_[child_region_id];
        node.parent_id = region_id;
        node.bounds = vecChild_AABB[i];

        node.stats.region_id = child_region_id;
                                         //  当前区域玩家数
        node.stats.area_size = node.bounds.GetArea();                //  区域面积（用于计算密度）
        node.stats.last_split_frame = 0;                             //  上次分裂帧号（防抖动）
        node.stats.last_merge_frame = 0;                             //  上次合并帧号（防抖动）
        node.depth = region_node.depth + 1;

        int width = node.bounds.GetWidth();
        int height = node.bounds.GetHeight();

        node.ClearPlayer();
        node.CreateStorage(aoi_type_, grid_size_);    
        

        MigrateEntitiesFromRegion(region_id, child_region_id);
        region_node.children.push_back(child_region_id);

        uint32_t delta_frame = abs(current_frame_ - (node.stats.last_merge_frame));
        //if(delta_frame >= merge_cooldown_frames_)
        {
            float fDensity = this->GetDensity(child_region_id);
            if(fDensity >= merge_threshold_)
            {
                split_region_id.push_back(child_region_id);
            }
        }
    }

    //std::cout << "DynamicAOI_V2::SplitRegion old_regions_num:=" << old_regions_num << " new_regions_num:=" << regions_.size() << std::endl;
    region_node.stats.last_split_frame = current_frame_;
    region_node.ClearStorage();

    for(const auto& split_region_id: split_region_id)
    {
        // 是否继续分裂
        SplitRegion(split_region_id);
    }
}
    
void DynamicAOI_V2::MergeRegion(uint32_t region_id)
{
    auto itr = regions_.find(region_id);
    if(itr == regions_.end())
    {
        return;
    }

    // 没有子节点，无法合并。
    RegionNode_V2& region_node = (itr->second);
    if(region_node.children.empty())
    {
        return;
    }
    
    /*
    uint32_t delta_frame = abs(current_frame_ - (region_node.stats.last_merge_frame));
    if(delta_frame < merge_cooldown_frames_)
    {
        return;
    }
    */

    
    // 判断合并条件是否满足，需要的是每一个children的密度都小于阈值
    for(int i = 0; i < region_node.children.size(); ++i)
    {
        // 四个节点都是叶子结点才可以。
        uint32_t child_region_id = region_node.children[i];
        auto itr_child = regions_.find(child_region_id); 
        if(itr_child == regions_.end())
        {
            continue;
        }

        // 叶子结点应该没有子节点，所以叶子结点的children为空。
        const RegionNode_V2& child_node = (itr_child->second);
        if(false == child_node.children.empty())
        {
            return;
        }
    
        float fDensity = this->GetDensity(child_region_id);
        if(fDensity >= merge_threshold_)
        {
            return;
        }
    }

    //std::cout << "DynamicAOI_V2::MergeRegion region_id:=" << region_id << std::endl;
    uint32_t old_regions_cout = regions_.size();
    // 所有子节点的密度都小于阈值，则可以合并。
    region_node.ClearPlayer(); 
    region_node.CreateStorage(aoi_type_, grid_size_);
    for(int i = 0; i < region_node.children.size(); ++i)
    {
        uint32_t child_region_id = region_node.children[i];

        MigrateEntitiesFromRegion(child_region_id, region_id);
        regions_.erase(child_region_id);
    }

    
    // std::cout << "DynamicAOI_V2::MergeRegion old_regions_num:=" << old_regions_cout << " new_regions_num:=" << regions_.size() << std::endl;
    region_node.stats.last_merge_frame = current_frame_;
    region_node.children.clear();

    // 判断是否需要向上合并。
    if(region_node.parent_id > 0)
    {
        MergeRegion(region_node.parent_id);
    }
   
}
    
void DynamicAOI_V2::CheckAndAdjust(uint32_t region_id)
{
    // 判断是否达到分裂或者合并的条件
    float fDensity = this->GetDensity(region_id);
   
    // 密度在合理的区间内
    // std::cout << "DynamicAOI_V2::CheckAndAdjust region_id:=" << region_id << " fDensity:=" << fDensity << std::endl;
    if(fDensity >= merge_threshold_ && fDensity <= split_threshold_)
    {
        return;
        
    }
    
    RegionNode_V2* ptr_region_node = this->GetRegion(region_id);
    if(!ptr_region_node)
    {
        return;            
    }

    if(fDensity > split_threshold_)
    {
        // 分裂
        /*
        uint32_t delta_frame = abs(current_frame_ - (ptr_region_node->stats.last_split_frame));
        if(delta_frame < split_cooldown_frames_)
        {
            return;
        }
        */

        SplitRegion(region_id);
    }
    else
    {
        // std::cout << "DynamicAOI_V2::CheckAndAdjust Merge Start" << std::endl;
        // 合并
        uint32_t parent_id = ptr_region_node->parent_id;
        if(0 == parent_id)
        {
            return; 
        }

        MergeRegion(parent_id);
        // std::cout << "DynamicAOI_V2::CheckAndAdjust Merge End" << std::endl;
    }
}

// ----- ----- ----------------------------------------------------------------------------------------------
// ----- -----------------------------------辅助查找---------------------------------------------
// ----- ----- ----------------------------------------------------------------------------------------------

uint32_t DynamicAOI_V2::FindLeafRegion(uint32_t region_id, int x, int y) const
{
    auto itr = regions_.find(region_id);
    if(itr == regions_.end())
    {
        return 0;
    }

    const RegionNode_V2& region_node = (itr->second);
    if(false == region_node.bounds.Contains(x, y))
    {
        return 0;
    }

    // 叶子结点
    if(region_node.children.empty())
    {
        return region_id;
    }
    else
    {
        // 找对应的子节点
        for(const auto& child_region_id : region_node.children)
        {
            uint32_t leaf_region_id = FindLeafRegion(child_region_id, x, y);
            if(leaf_region_id > 0)
            {
                return leaf_region_id;
            }
        }
    }
    
    // 异常，不应该执行到这儿。
    return 0;
}

// ----- ----- ----------------------------------------------------------------------------------------------
// ----- -----------------------------------辅助管理---------------------------------------------
// ----- ----- ----------------------------------------------------------------------------------------------

// 分配新区域 ID
uint32_t DynamicAOI_V2::AllocateRegionId()
{
    uint32_t new_region_id = next_region_id_;
    next_region_id_ += 1;
    return new_region_id;
}


// 实体迁移
void DynamicAOI_V2::MoveEntityToRegion(uint32_t entity_id, uint32_t new_region_id, int new_x, int new_y)
{

    auto itr_old = entity_to_region_.find(entity_id);
    if(itr_old != entity_to_region_.end())
    {
        DynamicAOI_V2::EntityRegionInfo& old_entity_info = (itr_old->second);
        RegionNode_V2& old_region_node = regions_[old_entity_info.region_id];

        if(old_entity_info.region_id == new_region_id)
        {
            old_entity_info.pos.first = new_x;
            old_entity_info.pos.second = new_y;

            old_region_node.storage->MoveEntity(entity_id, new_x, new_y);
            return;
        }

        old_region_node.RemovePlayer(entity_id);
    }
    
    DynamicAOI_V2::EntityRegionInfo& new_region_info = entity_to_region_[entity_id];
    new_region_info.region_id = new_region_id;
    new_region_info.pos.first = new_x;
    new_region_info.pos.second = new_y;
    /*
    if(entity_id == 1)
    {
        std::cout << "DynamicAOI_V2::MoveEntityToRegion player_id = 1 to new region_id:=" << new_region_id << std::endl;
    }
    */

    RegionNode_V2& new_region_node = regions_[new_region_id];
    new_region_node.AddPlayer(entity_id, new_x, new_y);
}


// ----- 实体迁移（分裂/合并时使用） 
void DynamicAOI_V2::MigrateEntitiesFromRegion(uint32_t from_region_id, uint32_t to_region_id)
{
    auto itr_from_region = regions_.find(from_region_id);
    if(itr_from_region == regions_.end())
    {
        return;
    }

    auto itr_to_region = regions_.find(to_region_id);
    if(itr_to_region == regions_.end())
    {
        return;
    }

    RegionNode_V2& from_region_node = (itr_from_region->second); 
    RegionNode_V2& to_region_node = (itr_to_region->second);

    std::vector<BaseEntityData> vecBaseEntityData = from_region_node.storage->GetAllEntities();
    // std::cout << "DynamicAOI_V2::MigrateEntitiesFromRegion old_region_id:=" << from_region_id << " to new region_id:=" << to_region_id << " total entity count:=" << vecBaseEntityData.size() << std::endl;
    for(auto& base_data : vecBaseEntityData)
    {
        auto itr_entity = entity_to_region_.find(base_data.id);
        if(itr_entity == entity_to_region_.end())
        {
            continue;
        }

        DynamicAOI_V2::EntityRegionInfo& entity_info = (itr_entity->second);
        if(false == to_region_node.bounds.Contains(entity_info.pos.first, entity_info.pos.second))
        {
            continue;
        }

        entity_info.region_id = to_region_id;
        //if(1 == base_data.id)
        {
            // std::cout << "DynamicAOI_V2::MigrateEntitiesFromRegion player_id=" << base_data.id << std::endl;
            //std::cout << "DynamicAOI_V2::MigrateEntitiesFromRegion player_id=1 from region_id:=" << from_region_id << " to region_id:=" << to_region_id << std::endl;
        }

        //std::cout << "DynamicAOI_V2::MigrateEntitiesFromRegion region_id:=" << to_region_id
        to_region_node.AddPlayer(base_data.id, entity_info.pos.first, entity_info.pos.second);
    }
}

std::vector<BaseEntityData> DynamicAOI_V2::GetAllEntities() const
{
    std::vector<BaseEntityData> vecAllEntity;
    vecAllEntity.reserve(entity_to_region_.size());
    for(const auto& [entity_id, info] : entity_to_region_)
    {
        BaseEntityData data;
        data.id = entity_id;
        data.x = info.pos.first;
        data.y = info.pos.second;
        
        vecAllEntity.emplace_back(std::move(data));
    }

    return vecAllEntity;
}



