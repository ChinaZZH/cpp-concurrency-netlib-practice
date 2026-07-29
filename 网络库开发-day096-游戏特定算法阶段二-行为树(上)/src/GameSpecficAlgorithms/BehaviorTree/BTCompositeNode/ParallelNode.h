#pragma once

#include "BTCompositeNode.h"

/**
 * 并行节点（Parallel）
 * 
 * 执行所有子节点，并根据两个掩码（all_mask / any_mask）决定整体成败。
 * 
 * 执行规则：
 * 1. 所有子节点都会被执行（每一帧都会更新）
 * 2. 先检查 all_mask：这些子节点必须全部成功（任一失败 → 整体失败）
 * 3. 再检查 any_mask：这些子节点任意一个成功 → 整体成功
 * 4. 否则返回 Failure
 * 
 * 构造约束：
 * - all_mask 和 any_mask 不能重叠（按位与必须为 0）
 * - 掩码的位数不能超过子节点数量
 */

class ParallelNode : public BTCompositeNode
{
public:
    /**
     * 构造函数
     * @param all_mask  必须全部成功的子节点掩码（位 i 对应 children[i]）
     * @param any_mask  任意一个成功即成功的子节点掩码
     */

    ParallelNode(uint32_t all_mask, uint32_t any_mask)
    : BTCompositeNode()
    , all_mask_(all_mask)
    ,any_mask_(any_mask)
    {
         // 校验1：掩码不能重叠
        if ((all_mask_ & any_mask_) != 0) {
            throw std::runtime_error("[Parallel] all_mask and any_mask must not overlap!");
        }
    }

    explicit ParallelNode()
    : BTCompositeNode()
    , all_mask_(0)
    ,any_mask_(0)
    {

    }

    virtual ~ParallelNode() = default;

    void InitMask(uint32_t all_mask, uint32_t any_mask)
    {
        all_mask_ = all_mask;
        any_mask_ = any_mask;

        // 校验1：掩码不能重叠
        if ((all_mask_ & any_mask_) != 0) {
            throw std::runtime_error("[Parallel] InitMask all_mask and any_mask must not overlap!");
        }
    }

    void TotalAll()
    {
        uint32_t max_mask = (1u << children_.size()) - 1;
        all_mask_ = max_mask;
        any_mask_ = 0;
    }

    void TotalAny()
    {
        uint32_t max_mask = (1u << children_.size()) - 1;
        all_mask_ = 0;
        any_mask_ = max_mask;
    }

    // 核心执行接口
    // - ctx: 状态上下文（与 FSM 共用 StateContext）
    // - delta_ms: 时间步长（毫秒）
    virtual BTStatus Execute(StateContext& ctx, float delta_ms)
    {
        // 在这边判断掩码是否有效
        {
            // 校验2：掩码位数不能超过子节点数量
            uint32_t max_mask = (1u << children_.size()) - 1;
            if((all_mask_ | any_mask_) & ~max_mask) 
            {
                throw std::runtime_error("[Parallel] Mask bits exceed child count!");
            }

            // 初始化子节点执行状态
            child_status_.resize(children_.size(), BTStatus::Failure);
        }

        // ================================================================
        // 1. 执行所有子节点（并行语义：全部更新）
        // ================================================================
        bool any_running = false;
        for(size_t i = 0; i < children_.size(); ++i)
        {
            if(nullptr == children_[i])
            {
                continue;
            }

            BTStatus resultStatus = children_[i]->Execute(ctx, delta_ms);
            child_status_[i] = resultStatus;
            if (resultStatus == BTStatus::Running) {
                any_running = true;
            }
        }

        // ================================================================
        // 2. 先检查 all_mask：必须全部成功，任一失败则整体失败
        // ================================================================
        for(size_t i = 0; i < children_.size(); ++i)
        {
            uint32_t bit_result = all_mask_ & (1 << i);
            if(bit_result <= 0)
            {
                continue;
            }

            // all_mask_必须全部成功才算成功，有任意一个结点失败就算失败。
            if(BTStatus::Failure == child_status_[i])
            {
                return BTStatus::Failure;
            }
        }

        // ================================================================
        // 3. 如果掩码要求的条件尚未满足，但有子节点还在 Running
        // ================================================================
        if (any_running) {
            return BTStatus::Running;
        }


        // all_mask_ 检查完了，如果没人any_mask_的掩码则直接返回结束。
        if(0 == any_mask_)
        {
            return BTStatus::Success;
        }

        // ================================================================
        // 4. 再检查 any_mask：任意一个成功则整体成功
        // ================================================================
        for(size_t i = 0; i < children_.size(); ++i)
        {
            uint32_t bit_result = any_mask_ & (1 << i);
            if(bit_result <= 0)
            {
                continue;
            }

            // any_mask_有任意一个结点成功就算成功，而且any在all之后已经经过all的验证了这边可以直接返回成功。
            if(BTStatus::Success == child_status_[i])
            {
                return BTStatus::Success;
            }
        }

        // ================================================================
        // 5. 所有掩码条件都不满足，整体失败
        // ================================================================
        return BTStatus::Failure;
    }

     // 重置节点状态（用于树的重置）
    virtual void ResetNode() override
    {
        BTCompositeNode::ResetNode();
        std::fill(child_status_.begin(), child_status_.end(), BTStatus::Failure);
    }

    // 获取节点名称（调试用）
    virtual std::string GetName() const override
    {
        return "Parallel";
    }

private:
    uint32_t all_mask_ = 0;                     // 必须全部成功的掩码
    uint32_t any_mask_ = 0;                     // 任意成功即成功的掩码
    std::vector<BTStatus> child_status_;        // 每个子节点的执行状态（用于 Running 持续跟踪）  
};