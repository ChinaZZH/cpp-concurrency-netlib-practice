#pragma once

#include <memory>
#include <functional>
#include <string>
#include <queue>
#include <chrono>
#include "Buffer.h"

class EventLoop;
class ClientSocket;
class HttpContext;
class Decoder;


class TcpConnection: public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, int fd, bool tcpClient = false);
    ~TcpConnection();

    void ConnectEstablished();  // 注册到EventLoop
    void Send(const std::string& strMessage);
    void Shutdown();

    EventLoop* GetLoop() const { return loop_; }
    int GetFd() const { return fd_; }

    using MessageCallBack = std::function<void(const std::shared_ptr<TcpConnection>&, std::string&)>;
    using CloseCallBack = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using ConnectionCallBack = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using HighWaterMarkCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using LowWaterMarkCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;

    void SetMessageCallBack(MessageCallBack cb) { messageCallBack_ = cb; }
    void SetCloseCallBack(CloseCallBack cb) { closeCallBack_ = cb; }
    void SetConnectionCallBack(ConnectionCallBack cb) { connectionCallBack_ = cb; }
    
    // 水位以及其相关变量
    void SetWaterMarkCallbacks(HighWaterMarkCallback highCb, LowWaterMarkCallback lowCb, size_t highMark, size_t lowMark = 0);
    bool IsPause() const { return pause_; }

    void WaitForWaterToLowMask(const std::string& strData);
    void WaterFromHighToLow();

    void ClosedConnection() { closed = true; }
    bool IsClosed() { return closed; }
    bool IsWriteClosed() { return IsClosed() || closedWrite_; }
    bool IsValid()  { return !IsWriteClosed(); }

    // 空闲连接超时检测，检测到了就关闭了
    std::chrono::steady_clock::time_point GetLastActiveTime() const { return lastActiveTime_; }
    void UpdateLastActiveTime()  { lastActiveTime_ = std::chrono::steady_clock::now(); }

    // 设置对应的解码器
    void SetDecoder(std::unique_ptr<Decoder> decoder);

private:
    void HandleRead();
    void HandleWrite();
    void HandleClose(std::string strCloseInfo);
    void HandleError();

    void SendOutput();  // 尝试发送outputBuffer_中的数据
    void ProcessInputBuffer();

    // 在Send或者SendOutput中调用内部函数
    void CheckWaterMark();

private:
    EventLoop* loop_;
    int fd_;
    std::unique_ptr<ClientSocket>  socket_;
    Buffer inputBuffer_;       // 输入缓冲区
    Buffer outputBuffer_;      // 输出缓冲区
    
    MessageCallBack messageCallBack_;
    CloseCallBack closeCallBack_;
    ConnectionCallBack connectionCallBack_;
    

    // 发送的时候设置高低水位，outputBuffer_.size() 超过高水位的时候就没法往 outputBuffer_进行Append数据
    // 当OutputBuffer的数据经过一系列的消化到达低水位的时候，就开启可以重新往OutputBuffer写数据
    size_t highWaterMark_ = 0;     // 高水位，高于这个这个高水位的值就不执行Send函数，禁止向OutputBuffer进行Append
    size_t lowWaterMark_ = 0;      // 低水位，低于这个这个低水位的值就重新开放执行Send函数的权限，开放向OutputBuffer进行Append

    HighWaterMarkCallback highWaterMarkCallback_;
    LowWaterMarkCallback  lowWaterMarkCallback_;
    bool pause_ = false;
    std::queue<std::string>  WaitLowWaterToSend_;
    
    bool closed = false;
    bool closedWrite_ = false;

    // 延迟删除的时候使用
    std::chrono::steady_clock::time_point lastActiveTime_;

    std::unique_ptr<Decoder> decoder_;
};