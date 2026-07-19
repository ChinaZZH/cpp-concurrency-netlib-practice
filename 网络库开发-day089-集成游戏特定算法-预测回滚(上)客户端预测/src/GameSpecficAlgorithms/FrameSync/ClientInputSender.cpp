#include "ClientInputSender.h"
#include "../../TcpConnection.h"
#include "../../Decoder/LengthAndTypePrefixDecoder.h"
#include "../../GameSpecficAlgorithms/GameServerMsgTypeDefine.h"
#include "../../../build/proto_gen/frame_sync.pb.h"

void ClientInputSender::Update(uint32_t client_frame) 
{
        ClientInput input;
        input.set_player_id(local_player_id_);
        input.set_frame_index(client_frame);
        input.set_move_x(current_x_);
        input.set_move_y(current_y_);
        input.set_client_seq(client_frame);
        
        std::string strContent = std::move(LengthAndTypePrefixDecoder::MakeRequestString(input.SerializeAsString(), GSMT_FrameClientInput));
        tcp_conn_->Send(strContent); // 使用你的 TcpConnection
}

void InitTcpConnection(std::shared_ptr<TcpConnection> tcpConnection);
void ClientInputSender::SetPosition(int newX, int newY)
{
    if(current_x_ != newX)
    {
        current_x_ = newX;
    }
    
    if(current_y_ != newY)
    {
        current_y_ = newY;
    }
}