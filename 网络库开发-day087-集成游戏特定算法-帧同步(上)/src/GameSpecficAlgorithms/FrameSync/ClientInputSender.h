#pragma once
#include <functional>
#include <string>
#include <atomic>
#include <memory>

class TcpConnection;
class ClientInputSender {
public:
    explicit ClientInputSender(uint32_t player_id, std::shared_ptr<TcpConnection> tcpConnection)
    :local_player_id_(player_id)
    , tcp_conn_(tcpConnection)
    { }

    void Update(uint32_t client_frame);

    void SetPosition(int newX, int newY);
    
private:
    uint32_t local_player_id_;
    std::shared_ptr<TcpConnection> tcp_conn_;
    std::atomic<int32_t> current_x_;
    std::atomic<int32_t> current_y_;
};