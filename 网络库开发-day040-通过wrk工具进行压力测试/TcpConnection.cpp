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
, pause_(false)
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
    auto channel = std::make_unique<Channel>(fd_);

    // 使用weak_ptr 避免循环引用
    std::weak_ptr<TcpConnection> weak_self = shared_from_this();
    channel->SetReadCallBack([weak_self]() { 
        auto self = weak_self.lock();
        if(self)
        {
            self->HandleRead(); 
        }
        
    });

    channel->SetWriteCallBack([weak_self]() { 
        auto self = weak_self.lock();
        if(self)
        {
            self->HandleWrite(); 
        }
    });

    channel->SetErrorCallBack([weak_self]() { 
        auto self = weak_self.lock();
        if(self)
        {
            self->HandleError(); 
        }
    });

    channel->EnableReading();
    channel->EnableET();
    loop_->AddChannel(std::move(channel));
}

    
void TcpConnection::Send(const std::string& strMessage)
{
    if(this->IsPause())
    {
        WaitForWaterToLowMask(strMessage);
        return;
    }

    outputBuffer_.Append(strMessage);
    CheckWaterMark();

    // 尝试立即发送
    SendOutput();
    // 如果发送后还有数据未发送完(比如发送缓冲区满)，需要注册EPOLLOUT
    if(outputBuffer_.ReadableBytes() > 0)
    {
        if(loop_)
        {
            // 开启写事件
            loop_->AddEventToUpdateChannel(fd_, EPOLLOUT);
        }
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
               ProcessInputBuffer();

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

        ProcessInputBuffer();
    }
}

void TcpConnection::HandleWrite()
{
    // 暂时留空，后续配合输出缓冲区实现
    auto self = shared_from_this();
    SendOutput();
    if(outputBuffer_.ReadableBytes() <= 0)
    {
        // 关闭写事件
        loop_->DelEventToUpdateChannel(fd_, EPOLLOUT);
    }
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


 // 尝试发送outputBuffer_中的数据
void TcpConnection::SendOutput()
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
            HandleClose();  // 不可恢复错误
        }else{
            // 缓冲区满，已经无法写入，等待EPOLL_OUT事件调度
            ;
        }

        return ;
    }

    // 对端关闭连接(优雅关闭)
    if(0 == n)
    {
        HandleClose();  // 对端关闭连接(优雅关闭)
        return;
    }

    CheckWaterMark();
    if(outputBuffer_.ReadableBytes() <= 0)
    {
        // 关闭写事件
        loop_->DelEventToUpdateChannel(fd_, EPOLLOUT);
    }
}


void TcpConnection::ProcessInputBuffer()
{
    if(!messageCallBack_ || inputBuffer_.ReadableBytes() <= 0)
    {
        return;
    }

    // 暂时不处理粘包
    /*
    while(true)
    {
        const char* crlf = static_cast<const char*>(memchr(inputBuffer_.Peek(), '\n', inputBuffer_.ReadableBytes()));
        if(!crlf)
        {
            break;
        }

        int len = crlf - inputBuffer_.Peek() + 1;
        std::string strLineMsg(inputBuffer_.Peek(), len);

        std::cout << "TcpConnection::ProcessInputBuffer  " << strLineMsg.c_str() << std::endl;
        messageCallBack_(shared_from_this(), strLineMsg);
        inputBuffer_.Retrieve(len);
    }
    */
    
    std::string strLineMsg = inputBuffer_.RetrieveAllAsString();
    messageCallBack_(shared_from_this(), strLineMsg);
}

 
void TcpConnection::SetWaterMarkCallbacks(HighWaterMarkCallback highCb, LowWaterMarkCallback lowCb, size_t highMark, size_t lowMark /*= 0*/)
{
    highWaterMarkCallback_ = highCb;
    lowWaterMarkCallback_ = lowCb;

    highWaterMark_ = highMark;
    lowWaterMark_ = lowMark;
}

// 在Send或者SendOutput中调用内部函数
void TcpConnection::CheckWaterMark()
{
    size_t readableBytes = outputBuffer_.ReadableBytes();
    if(!pause_ && highWaterMark_ > 0 && readableBytes > highWaterMark_ && highWaterMarkCallback_)
    {
        pause_ = true;
        highWaterMarkCallback_(shared_from_this());
    }

    if(pause_ && lowWaterMark_ > 0 && readableBytes <= lowWaterMark_ && lowWaterMarkCallback_)
    {
        pause_ = false;
        lowWaterMarkCallback_(shared_from_this());
    }
}


void TcpConnection::WaitForWaterToLowMask(const std::string& strData)
{
    if(this->IsPause())
    {
        WaitLowWaterToSend_.push(std::move(strData));
    }
}


void TcpConnection::WaterFromHighToLow()
{
    if(this->IsPause())
    {
        return ;
    }

    while(!WaitLowWaterToSend_.empty() && !this->IsPause())
    {
        std::string strData = WaitLowWaterToSend_.front();
        WaitLowWaterToSend_.pop();
        this->Send(strData);
    }
}