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
    outputBuffer_.Append(strMessage);
    SendAll();
}


void TcpConnection::Shutdown()
{
     HandleClose();
}


// 读事件处理(边缘触发)
void TcpConnection::HandleRead()
{
    auto self = shared_from_this();
    int savedErrno = 0;
    //char buffer[4096] = {0};
    while(true)
    {
        ssize_t n = inputBuffer_.ReadFd(fd_, &savedErrno);
        if(n < 0)
        {
            // EAGAIN 或 EWOULDBLOCK 表示本次数据读完了（非阻塞模式）
            // 读完了则进行写数据到对端
            if(savedErrno == EAGAIN || savedErrno == EWOULDBLOCK) {
                if(messageCallBack_ && inputBuffer_.ReadableBytes() > 0)
                {
                    std::string strMsg = inputBuffer_.RetrieveAllAsString();
                    messageCallBack_(shared_from_this(), strMsg);
                }

            }else{
                // 异常，则断开连接
                HandleError();
            }

            return;
        }else if(n == 0){
            // 对方关闭连接
            HandleClose();
            return;
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


void TcpConnection::SendAll()
{
    if(outputBuffer_.ReadableBytes() <= 0)
    {
        return ;
    }

    int nSavedErrno = 0;
    ssize_t n = outputBuffer_.WriteFd(fd_, &nSavedErrno);
    if(n < 0)
    {
        if(nSavedErrno != EAGAIN && nSavedErrno != EWOULDBLOCK)
        {
            HandleClose();
        }
    }
}