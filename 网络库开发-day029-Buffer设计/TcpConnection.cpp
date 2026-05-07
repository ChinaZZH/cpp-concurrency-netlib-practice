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
, socket_(std::make_unique<ClientSocket>(fd))
, inputBuffer_()
, outputBuffer_()
{
     socket_->SetNonBlock();
}


TcpConnection::~TcpConnection()
{
    if(fd_ > 0)
    {
        loop_->DelayRemoveQueue(fd_);
    }
}


// 注册到EventLoop
void TcpConnection::ConnectEstablished()  
{
    auto self = shared_from_this();

    auto channel = std::make_unique<Channel>(fd_);
    channel->SetReadCallBack([self]() { self->HandleRead(); } );
    channel->SetWriteCallBack([self]() { self->HandleWrite(); });
    channel->SetErrorCallBack([self]() { self->HandleError(); });

    channel->EnableReading();
    channel->EnableET();
    loop_->UpdateChannel(std::move(channel));
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
    auto self = shared_from_this();
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
                return;
            }
        }else if(n == 0){
            // 对方关闭连接
            HandleClose();
            return;
        }else{
            inputBuffer_.append(buffer, n);
        }
    }
}

void TcpConnection::HandleWrite()
{
    // 暂时留空，后续配合输出缓冲区实现
    auto self = shared_from_this();
}

void TcpConnection::HandleClose()
{
    auto self = shared_from_this();
    if(fd_ > 0)
    {
        loop_->DelayRemoveQueue(fd_);
    }
    
    if(closeCallBack_)
    {
        closeCallBack_(self);
    }
}

void TcpConnection::HandleError()
{
    HandleClose();
}

