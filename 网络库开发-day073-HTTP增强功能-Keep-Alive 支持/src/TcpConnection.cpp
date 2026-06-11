#include "TcpConnection.h"
#include "EventLoop.h"
#include "ClientSocket.h"
#include "Channel.h"
#include "Decoder/Decoder.h"
#include <iostream>
#include <cstring>
#include <sstream>
#include <unistd.h>


TcpConnection::TcpConnection(EventLoop* loop, int fd, bool tcpClient /*= false*/)
: loop_(loop)
, fd_(fd)
, socket_(std::make_unique<ClientSocket>(fd, tcpClient))
, inputBuffer_()
, outputBuffer_(8192)
, pause_(false)
,closed(false)
{
    socket_->SetNonBlock();

     // 更新tcpConnection的活跃时间
    this->UpdateLastActiveTime();
}


TcpConnection::~TcpConnection()
{
    loop_->AssertInLoopThread("TcpConnection::~TcpConnection");  // 确保是在主线程析构
}


// 注册到EventLoop
void TcpConnection::ConnectEstablished()  
{
    loop_->AssertInLoopThread("TcpConnection::ConnectEstablished");

    // std::cout << "connectEstablished: fd=" << fd_ << " on thread " << std::this_thread::get_id() << std::endl;

    socket_->DisableNagele();

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

    channel->SetCloseCallBack([weak_self]() { 
        auto self = weak_self.lock();
        if(self)
        {
            self->HandleClose("EPOLLHUP Channel CallBack"); 
        }
    });

    channel->EnableReading();
    channel->EnableET();
    channel->EnableEvent(EPOLLRDHUP);
    loop_->AddChannel(std::move(channel), "TcpConnection::ConnectEstablished");

     // 更新tcpConnection的活跃时间
    this->UpdateLastActiveTime();

    if(connectionCallBack_)
    {
        connectionCallBack_(shared_from_this());
    }
}

    
void TcpConnection::Send(const std::string& strMessage)
{
    loop_->AssertInLoopThread("TcpConnection::Send");
    if(this->IsWriteClosed())
    {
        return;
    }

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
    loop_->AssertInLoopThread("TcpConnection::Shutdown");
    if(IsClosed() || closedWrite_)
    {
        return;
    }

    if(::shutdown(fd_, SHUT_WR) < 0)
    {
        std::cerr << "shutdown error fd:=" << fd_ << std::endl;
    }
    else{
        closedWrite_ = true;
        // std::cout << "close write fd:=" << fd_ << std::endl;
    }
}


// 读事件处理(边缘触发)
void TcpConnection::HandleRead()
{
    if(IsClosed())
    {
        return;
    }

    loop_->AssertInLoopThread("TcpConnection::HandleRead");

    // 更新连接的活跃时间
    UpdateLastActiveTime();

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
            HandleClose("TcpConnection::HandleRead");
            return;
        }

        ProcessInputBuffer();
    }
}

void TcpConnection::HandleWrite()
{
    if(this->IsWriteClosed())
    {
        return;
    }

    // 暂时留空，后续配合输出缓冲区实现
    auto self = shared_from_this();
    SendOutput();
    if(outputBuffer_.ReadableBytes() <= 0)
    {
        // 关闭写事件
        loop_->DelEventToUpdateChannel(fd_, EPOLLOUT);
    }
}

void TcpConnection::HandleClose(std::string strCloseInfo)
{
    if(IsClosed())
    {
        return;
    }

    loop_->AssertInLoopThread("TcpConnection::HandleClose");  // 确保在 对应的工作线程

    // 需要的时候开启，不需要的时候注释
    //std::cout << "TcpConnection::HandleClose fd:=" << fd_  << "  Close reason:="  << strCloseInfo.c_str() << std::endl;
    auto self = shared_from_this();

    if(fd_ > 0)
    {    
        loop_->DelayRemoveQueue(fd_, socket_->IsTcpClient());
    }
    
    
    if(closeCallBack_)
    {
        closeCallBack_(self);
    }
}

void TcpConnection::HandleError()
{
    if(IsClosed())
    {
        return;
    }

    HandleClose("TcpConnection::HandleError");
}


 // 尝试发送outputBuffer_中的数据
void TcpConnection::SendOutput()
{
    loop_->AssertInLoopThread("TcpConnection::SendOutput");
    if(this->IsWriteClosed())
    {
        return;
    }

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
            HandleClose("TcpConnection::SendOutput n < 0");  // 不可恢复错误
        }else{
            // 缓冲区满，已经无法写入，等待EPOLL_OUT事件调度
            ;
        }

        return ;
    }

    // 对端关闭连接(优雅关闭)
    if(0 == n)
    {
        HandleClose("TcpConnection::SendOutput n == 0");  // 对端关闭连接(优雅关闭)
        return;
    }

    // 更新连接的活跃时间
    UpdateLastActiveTime();

    CheckWaterMark();
    if(outputBuffer_.ReadableBytes() <= 0)
    {
        // 关闭写事件
        loop_->DelEventToUpdateChannel(fd_, EPOLLOUT);

        // 数据写完后 http conneciton:close 需要主动关闭连接
        if(close_when_write_finish_ && WaitLowWaterToSend_.empty())
        {
            std::cout << "TcpConnection::SendOutput close_when_write_finish_ to Shutdown\n";
            Shutdown();
        }
    }
}


void TcpConnection::ProcessInputBuffer()
{
    if(inputBuffer_.ReadableBytes() <= 0)
    {
        return;
    }
    
    // 没有解码器的情况下默认全部读取(不同的解码器用不同的方式处理粘包)
    if(!decoder_)
    {
        if(messageCallBack_)
        {
            std::string strLineMsg = inputBuffer_.RetrieveAllAsString();
            messageCallBack_(shared_from_this(), strLineMsg);
        }
        
    }
    else{
        std::string msg;
        while(decoder_->Decode(inputBuffer_, msg))
        {
            if(messageCallBack_)
            {
                messageCallBack_(shared_from_this(), msg);
            }
        }
    }
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


void TcpConnection::SetDecoder(std::unique_ptr<Decoder> decoder)
{
    decoder_ = std::move(decoder);
}

// 主动关闭接口
// http client 发送短连接 connection:close 的时候需要等数据全部发送完的时候进行shutdown
void TcpConnection::CloseWhenWriteFinish()
{
    loop_->AssertInLoopThread("TcpConnection::SendOutput");
    if(outputBuffer_.ReadableBytes() <= 0 && WaitLowWaterToSend_.empty())
    {
        Shutdown();
        std::cout << "TcpConnection::CloseWhenWriteFinish Shutdown\n";
    }
    else
    {
        close_when_write_finish_ = true;
        // 确保写事件是开启的（否则不会触发 SendOutput）
        bool bCheck = loop_->CheckEvetFromChannel(fd_, EPOLLOUT);
        if(!bCheck) 
        {
            // 写入的时候有开启epoll_out写事件，一般不错到这里。打个日志吧。
            std::cout << "Warning Error!!! TcpConnection::CloseWhenWriteFinish" << std::endl;
            loop_->AddEventToUpdateChannel(fd_, EPOLLOUT);
        }

        std::cout << "TcpConnection::CloseWhenWriteFinish close_when_write_finish_ true\n";
    }

}