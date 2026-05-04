#include "TcpConnection.h"
#include "EventLoop.h"
#include "ClientSocket.h"
#include "Channel.h"
#include <iostream>
#include <cstring>
#include <unistd.h>

TcpConnection::TcpConnection(EventLoop* loop, int fd)
: loop_(loop)
, fd_(fd)
, channel_(std::make_unique<Channel>(fd))
, socket_(std::make_unique<ClientSocket>(fd))
, inputBuffer_()
, outputBuffer_()
{
    channel_->SetReadCallBack(std::bind(&TcpConnection::HandleRead, this));
    channel_->SetWriteCallBack(std::bind(&TcpConnection::HandleWrite, this));
    channel_->SetErrorCallBack(std::bind(&TcpConnection::HandleError, this));
    socket_->SetNonBlock();
}


TcpConnection::~TcpConnection()
{
    if(channel_)
    {
        loop_->RemoveChannel(channel_.get());
    }
}


// 注册到EventLoop
void TcpConnection::ConnectEstablished()  
{
    channel_->EnableReading();
    channel_->EnableET();
    loop_->UpdateChannel(channel_.get());
}

    
void TcpConnection::Send(const std::string& strMessage)
{
    bool bResult = socket_->Write(strMessage.c_str(), strMessage.length());
    bool bErrorBuff = (errno == EAGAIN || errno == EWOULDBLOCK);
    if(!bResult && !bErrorBuff)
    {
        HandleClose();
    }
}


void TcpConnection::Shutdown()
{
     HandleClose();
}


// 读事件处理(边缘触发)
void TcpConnection::HandleRead()
{
    char buffer[4096] = {0};
    while(true)
    {
        memset(buffer, 0, sizeof(buffer));
        errno = 0;
        ssize_t n = socket_->Read(buffer, sizeof(buffer));
        int err = errno;   // 立即保存 errno
        if(n < 0)
        {
            // EAGAIN 或 EWOULDBLOCK 表示本次数据读完了（非阻塞模式）
            // 读完了则进行写数据到对端
            if(err == EAGAIN || err == EWOULDBLOCK) {
                if(messageCallBack_)
                {
                    std::string strMsg = std::move(inputBuffer_);
                    inputBuffer_.clear();
                    messageCallBack_(shared_from_this(), strMsg);
                }

                break;
            }else{
                // 异常，则断开连接
                HandleError();
                break;
            }
        }else if(n == 0){
            // 对方关闭连接
            HandleClose();
            break;
        }else{
            inputBuffer_.append(buffer, n);
        }
    }
}

void TcpConnection::HandleWrite()
{
    // 暂时留空，后续配合输出缓冲区实现
}

void TcpConnection::HandleClose()
{
    if(channel_)
    {
        loop_->RemoveChannel(channel_.get());
        channel_.reset();
    }
    
    if(closeCallBack_)
    {
        closeCallBack_(shared_from_this());
    }
}

void TcpConnection::HandleError()
{
    HandleClose();
}

