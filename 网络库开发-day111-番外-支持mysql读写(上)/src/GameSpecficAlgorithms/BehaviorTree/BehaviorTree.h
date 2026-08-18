#pragma once

enum class BTStatus {
    Success,            // 节点执行成功
    Failure,            // 节点执行失败
    Running,            // 节点正在执行中，下一帧继续
};