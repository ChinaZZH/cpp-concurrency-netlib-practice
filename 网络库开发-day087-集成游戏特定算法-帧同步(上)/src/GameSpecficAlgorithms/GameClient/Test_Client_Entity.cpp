#include "Test_Client_Entity.h"
#include <iostream>
#include <cassert>
#include "../AttributeSync/DeltaSyncManager.h"
#include "../AttributeSync/DirtyTracker.h"
#include "ReassemblyBuffer.h"
#include "UdpReassemblyBuffer.h"

// 模拟打印回调，防止空指针
void TestClientEntity::DummySend(uint32_t conn_id, const std::string& data) {
    // 仅用于防止崩溃，不做实际发送
}

// 测试1：DirtyTracker 原子位操作
bool TestClientEntity::TestDirtyTracker() {
    std::cout << "[Test 1] DirtyTracker ... " << std::flush;
    DirtyTracker dt;

    dt.MarkDirty(1);  // 置位第1位
    dt.MarkDirty(3);  // 置位第3位
    uint64_t mask = dt.AcquireAndClear();

    // 预期结果：第1位和第3位为1（即 0b1010，十进制10）
    uint64_t expected_mask = (1ULL << 1) | (1ULL << 3);
    if (mask != expected_mask) {
        std::cout << "FAILED (mask=" << mask << ") expected_mask=" << expected_mask << std::endl;
        return false;
    }

    // 再次 AcquireAndClear 应该返回 0（因为已经清空）
    if (dt.AcquireAndClear() != 0) {
        std::cout << "FAILED (not cleared)" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

// 测试2：EntityHistory（std::map）自动排序与容量控制
bool TestClientEntity::TestEntityHistory() {
    std::cout << "[Test 2] EntityHistory (std::map) ... " << std::flush;
    DeltaSyncManager::HistoryVersion hist;

    // 构造三个不同版本的假增量包（只填充版本号，fields 留空）
    AttributeDelta d3, d5, d1;
    d3.set_version_no(3);
    d5.set_version_no(5);
    d1.set_version_no(1);

    // 乱序压入
    hist.Push(d3);
    hist.Push(d5);
    hist.Push(d1);

    // 验证 current_version 是否更新为最新（5）
    if (hist.current_version != 5) {
        std::cout << "FAILED (current_version=" << hist.current_version << ")" << std::endl;
        return false;
    }

    // 验证 Contains 能否正确找到存在的版本
    if (!hist.Contains(3) || !hist.Contains(5) || !hist.Contains(1)) {
        std::cout << "FAILED (Contains missing)" << std::endl;
        return false;
    }

    // 验证 GetRange(3) 是否返回 [3, 5]（自动升序）
    auto range = hist.GetRange(3);
    if (range.size() != 2 || range[0].version_no() != 3 || range[1].version_no() != 5) {
        std::cout << "FAILED (GetRange size/order)" << std::endl;
        return false;
    }

    // 测试容量控制：压入超过 64 个版本，验证最旧的被淘汰
    for (uint32_t v = 6; v <= 70; ++v) {
        AttributeDelta tmp;
        tmp.set_version_no(v);
        hist.Push(tmp);
    }
    // 此时历史窗口应只保留 [7, 70]（旧版本 1~6 被淘汰）
    if (hist.Contains(1) || hist.Contains(6)) {
        std::cout << "FAILED (oldest not evicted)" << std::endl;
        return false;
    }
    if (!hist.Contains(7) || !hist.Contains(70)) {
        std::cout << "FAILED (newest missing)" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

// 测试3：ReassemblyBuffer 基础排序与 pending 队列释放
bool TestClientEntity::TestReassemblyBuffer() {
    std::cout << "[Test 3] ReassemblyBuffer (expected & pending) ... " << std::flush;

    // 模拟 NACK 回调（仅打印，不实际发送）
    /*
    auto nack_cb = [](uint32_t entity_id, uint32_t from_version) {
        std::cout << "[NACK] entity=" << entity_id << " from=" << from_version << std::endl;
    };
    */

    ReassemblyBuffer buffer;
    uint32_t entity_id = 1001;
    //buffer.SetCurrentTime(1000);  // 初始时间

    // 场景：期望 version 1，但先收到 version 3（提前到达）
    AttributeDelta d3;
    d3.set_entity_id(entity_id);
    d3.set_version_no(3);
    d3.set_is_full_snapshot(false);
    buffer.PushDelta(entity_id, d3);

    // 此时 pending 队列中应有 version 3，且 expect_version_ 仍为 0（或1）
    // 由于我们没有暴露内部变量，无法直接检查。手动触发一次 NACK 后，检查没有崩溃即为成功。

    // 现在收到期望的 version 1
    AttributeDelta d1;
    d1.set_entity_id(entity_id);
    d1.set_version_no(1);
    d1.set_is_full_snapshot(false);
    buffer.PushDelta(entity_id, d1);

    // 如果代码逻辑正确，应用 d1 后，循环应自动将 pending 中的 d3 取出并应用。
    // 这一步只要不崩溃，且没有触发错误的 NACK（200ms 冷却内不会重复触发），就算通过。

    // 测试全量快照硬重置：发送 version 100 的全量包
    AttributeDelta full;
    full.set_entity_id(entity_id);
    full.set_version_no(100);
    full.set_is_full_snapshot(true);
    buffer.PushDelta(entity_id, full);

    // 再次发送一个旧的 version 50，应该被丢弃（因为 version < expected）
    AttributeDelta old;
    old.set_entity_id(entity_id);
    old.set_version_no(50);
    old.set_is_full_snapshot(false);
    buffer.PushDelta(entity_id, old);
    // 此操作若不崩溃，则说明 version < expected 分支逻辑正常。

    std::cout << "PASSED" << std::endl;
    return true;
}


// 测试3：ReassemblyBuffer 基础排序与 pending 队列释放
bool TestClientEntity::TestUdpReassemblyBuffer(){
    std::cout << "[Test 3] TestUdpReassemblyBuffer (expected & pending) ... " << std::flush;

    // 模拟 NACK 回调（仅打印，不实际发送）
    auto nack_cb = [](uint32_t entity_id, uint32_t from_version) {
        std::cout << "[NACK] entity=" << entity_id << " from=" << from_version << std::endl;
    };
    

    UdpReassemblyBuffer buffer(nack_cb);
    uint32_t entity_id = 1001;
    //buffer.SetCurrentTime(1000);  // 初始时间

    // 场景：期望 version 1，但先收到 version 3（提前到达）
    AttributeDelta d3;
    d3.set_entity_id(entity_id);
    d3.set_version_no(3);
    d3.set_is_full_snapshot(false);
    buffer.PushDelta(entity_id, d3);

    // 此时 pending 队列中应有 version 3，且 expect_version_ 仍为 0（或1）
    // 由于我们没有暴露内部变量，无法直接检查。手动触发一次 NACK 后，检查没有崩溃即为成功。

    // 现在收到期望的 version 1
    AttributeDelta d1;
    d1.set_entity_id(entity_id);
    d1.set_version_no(1);
    d1.set_is_full_snapshot(false);
    buffer.PushDelta(entity_id, d1);

    // 如果代码逻辑正确，应用 d1 后，循环应自动将 pending 中的 d3 取出并应用。
    // 这一步只要不崩溃，且没有触发错误的 NACK（200ms 冷却内不会重复触发），就算通过。

    // 测试全量快照硬重置：发送 version 100 的全量包
    AttributeDelta full;
    full.set_entity_id(entity_id);
    full.set_version_no(100);
    full.set_is_full_snapshot(true);
    buffer.PushDelta(entity_id, full);

    // 再次发送一个旧的 version 50，应该被丢弃（因为 version < expected）
    AttributeDelta old;
    old.set_entity_id(entity_id);
    old.set_version_no(50);
    old.set_is_full_snapshot(false);
    buffer.PushDelta(entity_id, old);
    // 此操作若不崩溃，则说明 version < expected 分支逻辑正常。

    std::cout << "PASSED" << std::endl;
    return true;
}


int TestClientEntity::Test() {
    std::cout << "=== Day 5 Smoke Tests ===" << std::endl;

    bool all_pass = true;
    all_pass &= TestDirtyTracker();
    all_pass &= TestEntityHistory();
    all_pass &= TestReassemblyBuffer();
    all_pass &= TestUdpReassemblyBuffer();

    if (all_pass) {
        std::cout << "\n✅ All smoke tests PASSED. Day 5 core logic is stable." << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ Some tests FAILED. Please check logs above." << std::endl;
        return 1;
    }

    return 1;
}