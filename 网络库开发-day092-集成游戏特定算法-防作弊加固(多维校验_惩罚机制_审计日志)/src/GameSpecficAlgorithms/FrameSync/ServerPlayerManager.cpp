#include "ServerPlayerManager.h"
#include "../GameServer.h"
#include "AuditLog.h"

ServerPlayerManager::ServerPlayerManager(SendCorrectionCallback cb)
:send_correction_cb_(cb)
,server_frame_index_(0)
{
    audit_logger_ = std::make_shared<AuditLogger>();
    audit_logger_->Init("logs/anticheat.csv", true);  // // 异步模式
}

 
// 1. 注册玩家（当玩家加入游戏时调用）
void ServerPlayerManager::AddPlayer(uint32_t player_id)
{
    auto itr = players_.find(player_id);
    if(itr != players_.end())
    {
        printf("[Server] Player %u already exists, skipping AddPlayer.\n", player_id);
        return;
    }


    // 初始化本地状态为原点
    ServerPlayerState initState;
    initState.x = Fixed::Zero();
    initState.y = Fixed::Zero();

    initState.vx = Fixed::Zero();
    initState.vy = Fixed::Zero();

    initState.state = 0;
    initState.hp = 100;
    initState.client_frame = 0;
    players_[player_id] =  initState;
    std::cout << "[Server] Player " << player_id << " registered." << std::endl;
}

// 2. 移除玩家（玩家退出时调用）
void ServerPlayerManager::RemovePlayer(uint32_t player_id)
{
    players_.erase(player_id);
    pending_inputs_.erase(player_id);
}
    
// 3. 获取某个玩家的权威状态（用于下发给客户端）
bool ServerPlayerManager::GetPlayerState(uint32_t player_id, ServerPlayerState& out) const
{
    auto itr = players_.find(player_id);
    if(itr == players_.end())
    {
        return false;
    }

    out = (itr->second);
    return true;
}


// 内部模拟函数（与客户端完全一致） 
ServerPlayerState ServerPlayerManager::Simulate(const ServerPlayerState& current, const ClientInput& input, uint32_t delta_ms)
{
    ServerPlayerState newPlayerState = current;

    float fSpeed = 0.1f; // 每毫秒移动速度(固定速率)
    newPlayerState.x += Fixed(input.move_x() * fSpeed * delta_ms);
    newPlayerState.y += Fixed(input.move_y() * fSpeed * delta_ms);
    newPlayerState.client_frame = input.client_seq();
    return newPlayerState;
}


 // 由 TcpServer 的 OnMessage 调用，将客户端上报的输入暂存起来
void ServerPlayerManager::SumbitInput(uint32_t player_id, const ClientInput& input)
{
    PendingInput pendingData;
    pendingData.input = input;
    pendingData.client_frame = input.client_seq();
    pending_inputs_[player_id] = pendingData;
}


// 4. 核心：每帧 Tick，推进所有玩家的状态
//    参数：当前服务端帧号，所有玩家在这一帧的输入列表，时间步长(ms)
void ServerPlayerManager::Tick(uint32_t delta_ms)
{
    std::unordered_map<uint32_t, PendingInput>  inputs; 
    inputs.swap(pending_inputs_);
    

    server_frame_index_ += 1;

    // 遍历所有玩家
    for(auto& [player_id, playerState] : players_)
    {
        // ===== 1. 提取输入（含预测坐标） =====
        ClientInput input;
        uint32_t clientFrame = 0;
        bool hasPrediction = false;
        Fixed predictedX, predictedY;
        auto itr = inputs.find(player_id);
        if(itr != inputs.end())
        {
            hasPrediction = true;
            auto& clientInput = (itr->second).input;
            clientInput.set_frame_index(server_frame_index_);

            input = clientInput;
            clientFrame = (itr->second).client_frame;
            predictedX = Fixed::FromRaw(input.predicted_x());
            predictedY = Fixed::FromRaw(input.predicted_y());
        }
        else
        {
             // 空输入：所有操作归零
            input.set_player_id(player_id);
            input.set_frame_index(server_frame_index_); // 服务端帧号，客户端帧号走client_seq
            input.set_move_x(0);
            input.set_move_y(0);
            input.set_attack(false);
            input.set_skill_id(0);
        }

        // ===== 2. 执行权威模拟 =====
        ServerPlayerState oldPlayerState = playerState;
        playerState = Simulate(playerState, input, delta_ms);
        
        // 有客户端输入才去记录。
        this->PushHistory(player_id, clientFrame, server_frame_index_, playerState);
        
        // 增加校验
        this->CheckAntiCheat(player_id, playerState, delta_ms, clientFrame);
        bool bPunishment = this->PunishmentBytViolation(player_id, playerState);
        if(nullptr == send_correction_cb_)
        {
            //std::cout << "[Server] send_correction_cb_ is nullptr.\n";
            continue;
        }

        
        // ===== 3. 校验对比（如果客户端有上报预测位置） =====
        // 已经受到违规惩罚，则不必走这个强制矫正
        if(false == bPunishment && hasPrediction)
        {
            Fixed dx = playerState.x - predictedX;
            Fixed dy = playerState.y - predictedY;
            if(dx < Fixed::Zero())
            {
                dx = -dx;
            }

            if(dy < Fixed::Zero())
            {
                dy = -dy;
            }


            // 【正确写法】设定阈值为 0.02 单位，MOBA/FPS 通用
            Fixed threshold = Fixed(0.02f);
            if(dx > threshold || dy > threshold)
            {
                // 【关键】只记录日志，暂不下发矫正
                /*
                printf("[Server] Player %u MISMATCH! Server=(%.4f, %.4f), Client=(%.4f, %.4f), Diff=(%.4f, %.4f)\n",
                       player_id,
                       playerState.x.ToDouble(), playerState.y.ToDouble(),
                       predictedX.ToDouble(), predictedY.ToDouble(),
                       dx.ToDouble(), dy.ToDouble());
                */

               std::cout << "[Server] MISMATCH! Sending correction.\n";

               ServerCorrection correction;
               correction.set_player_id(player_id);
               correction.set_client_frame(clientFrame);
               correction.set_authoritative_x(playerState.x.Raw());
               correction.set_authoritative_y(playerState.y.Raw());

               std::string strData;
               correction.SerializeToString(&strData);
               send_correction_cb_(player_id, strData);

            }
            else{
                std::cout << "[Server] OK! Sending correction.\n";
            }
        }
    }
}



// 构建全量快照回复包（只读）
bool ServerPlayerManager::BuildSnapShotReply(uint32_t player_id, SnapshotReply& reply)
{
    auto itr = players_.find(player_id);
    if(itr == players_.end())
    {
        return false;
    }

    ServerPlayerState playerState = (itr->second);     
    reply.set_player_id(player_id);
    reply.set_client_frame_hint(playerState.client_frame);
    reply.set_server_frame_index(server_frame_index_);
    reply.set_x(playerState.x.Raw());
    reply.set_y(playerState.y.Raw());
    reply.set_vx(playerState.vx.Raw());
    reply.set_vy(playerState.vy.Raw());
    reply.set_hp(playerState.hp);
    reply.set_state(playerState.state);
    return true;
}


// 按客户端帧号查询历史状态
bool ServerPlayerManager::GetHistoryByServerFrame(uint32_t player_id, uint32_t server_frame_index, ServerPlayerState& out)
{
    auto itr = histories_.find(player_id);
    if(itr == histories_.end())
    {
        return false;
    }

    const ServerPlayerManager::PlayerHistory& playerHistory = (itr->second);
    return playerHistory.FindClientFrame(server_frame_index, out);
}


// 推入历史数据
void ServerPlayerManager::PushHistory(uint32_t player_id, uint32_t client_frame, uint32_t server_frame, const ServerPlayerState& state)
{
    if(server_frame <= 0)
    {
        return ;
    }

    HistoryEntry entry;
    entry.client_frame = client_frame;
    entry.server_frame = server_frame;
    entry.state = state;

    ServerPlayerManager::PlayerHistory& playerHistory = histories_[player_id];
    playerHistory.Push(entry);
}


// 判断是否可以攻击，主要用于判断攻击的频率
bool ServerPlayerManager::CheckAttackTarget(uint32_t player_id, uint32_t server_frame_index, uint32_t target_id)
{
    // ===== 1. 攻击频率校验 =====
    AntiCheatState& acState = anti_cheat_states_[player_id];
    if(acState.last_attack_frame > 0)
    {
        // 发上来的服务端帧号小于当前的服务端帧号，则说明发上来的数据错误
        uint32_t diff_frame_num = server_frame_index - acState.last_attack_frame;
        if((server_frame_index < acState.last_attack_frame) || (diff_frame_num < MIN_ATTACK_INTERVAL_FRAMES))
        {
            // 技能处于cd时间内，不让攻击
            acState.violation_count += 1;
            //printf("[AntiCheat] Player %u attack too frequent! diff=%u, min=%u\n", player_id, diff_frame_num, MIN_ATTACK_INTERVAL_FRAMES);
            LogViolation(
                player_id, 
                "attack_freq", 
                std::to_string(diff_frame_num), 
                std::to_string(MIN_ATTACK_INTERVAL_FRAMES),
                acState.violation_count, 
                "ignored"
            );

            return false;
        }
    }


    acState.last_attack_frame = server_frame_index_;
    return true;
}


bool ServerPlayerManager::CheckAntiCheat(uint32_t player_id, ServerPlayerState& playerState, uint32_t delta_ms, uint32_t client_frame_index)
{
    // 客户端没有发clientInput包上来。
    if(client_frame_index <= 0)
    {
        return true;
    }

    // 1. 速度校验
    AntiCheatState& acState = anti_cheat_states_[player_id];
    if(acState.has_prev_state)
    {
        Fixed diff_X = playerState.x - acState.prevState.x;
        Fixed diff_Y = playerState.y - acState.prevState.y;
        Fixed sqrtDist = diff_X * diff_X + diff_Y * diff_Y;

        float dist = (GameServer::MAX_MOVE_SPEED / 1000.00f);
        dist = dist * delta_ms;
        Fixed maxDist(dist);
        Fixed maxSqrtDist = (maxDist * maxDist);
        if(sqrtDist > maxSqrtDist)
        {
            acState.violation_count += 1;
            // 强制拉回
            playerState.x = acState.prevState.x;
            playerState.y = acState.prevState.y;

            LogViolation(
                player_id, 
                "speed", 
                std::to_string(FixedMath::FixedSqrt(sqrtDist).ToDouble()), 
                std::to_string(maxDist.ToDouble()),
                acState.violation_count, 
                "correction"
            );
        }
    }


    // 3. 帧号连续性校验
    if(acState.last_client_frame > 0)
    {
        // 帧号出错或者跳帧了
        if(client_frame_index <= acState.last_client_frame || client_frame_index - acState.last_client_frame > 1)
        {
            acState.violation_count += 1;

            LogViolation(
                player_id, 
                "frame_skip", 
                std::to_string(client_frame_index), 
                std::to_string(acState.last_client_frame + 1),
                acState.violation_count, 
                "warning"
            );
        }
    }

    // 更新上一个帧号。
    if(client_frame_index > acState.last_client_frame)
    {
        acState.last_client_frame = client_frame_index;
    }

    acState.prevState = playerState;
    acState.has_prev_state = true;
    return true;
}


// 获取并重置违规计数（用于外部模块判断是否触发惩罚）
uint32_t ServerPlayerManager::GetAndResetViolationCount(uint32_t player_id)
{
    auto itr = anti_cheat_states_.find(player_id);
    if(itr == anti_cheat_states_.end())
    {
        return 0;
    }

    AntiCheatState& acState = (itr->second);
    uint32_t violationCount = acState.violation_count;

    acState.violation_count = 0;
    return violationCount;
}


bool ServerPlayerManager::PunishmentBytViolation(uint32_t player_id, const ServerPlayerState& playerState)
{
    if(playerState.client_frame <= 0)
    {
        return false;
    }

    auto itr = anti_cheat_states_.find(player_id);
    if(itr == anti_cheat_states_.end())
    {
        return false;
    }

    // 1. 警告（仅日志，已在 CheckAntiCheat 1 中输出） 无需额外操作
    bool bPunishment = false;
    AntiCheatState& acState = (itr->second);
    uint32_t violation_count = acState.violation_count;

    // 2. 强制矫正（violation_count >= 3）
    if(violation_count >= VIOLATION_CORRECT_THRESHOLD)
    {
        bPunishment = true;

        // 构造矫正包，强制拉回权威位置
        //printf("[AntiCheat] Player %u corrected (violation count: %u)\n", player_id, violation_count);
        if(send_correction_cb_)
        {
            ServerCorrection correction;
            correction.set_player_id(player_id);
            correction.set_client_frame(playerState.client_frame);
            correction.set_authoritative_x(playerState.x.Raw());
            correction.set_authoritative_y(playerState.y.Raw());

            std::string strData;
            correction.SerializeToString(&strData);
            send_correction_cb_(player_id, strData);
        }

        LogViolation(
                player_id, 
                "penalty_correction", 
                "", 
                "",
                acState.violation_count, 
                "correction_sent"
            );
    }


    // 3. 踢出（violation_count >= 5）
    if(violation_count >= VIOLATION_KICK_THRESHOLD)
    {
        bPunishment = true;
        acState.violation_count = 0; // 重置违规计数，防止重复踢出

        //printf("[AntiCheat] Player %u kicked (violation count: %u)\n", player_id, violation_count);
        if(kick_callback_)
        {
            kick_callback_(player_id);
        }

        LogViolation(
                player_id, 
                "penalty_kick", 
                "", 
                "",
                acState.violation_count, 
                "kick"
            );
    }


    // 4. 封禁（violation_count >= 10，可选）
    if(violation_count >= VIOLATION_BAN_THRESHOLD)
    {
        bPunishment = true;
        acState.violation_count = 0;    // 重置违规计数，防止重复踢出


        //printf("[AntiCheat] Player %u banned (violation count: %u)\n", player_id, violation_count);
        if(ban_callback_)
        {
            ban_callback_(player_id);
        }

        LogViolation(
                player_id, 
                "ban_kick", 
                "", 
                "",
                acState.violation_count, 
                "ban"
            );
    }

    return bPunishment;
}


void ServerPlayerManager::LogViolation(uint32_t player_id, const std::string& type,
                      const std::string& current, const std::string& threshold,
                      uint32_t count, const std::string& penalty)
{

    if (!audit_logger_) 
    {
        return;
    }

    AuditEntry entry;
    entry.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    entry.player_id = player_id;
    entry.violation_type = type;
    entry.current_value = current;
    entry.threshold_value = threshold;
    entry.violation_count = count;
    entry.penalty_action = penalty;

    audit_logger_->Log(entry);
}