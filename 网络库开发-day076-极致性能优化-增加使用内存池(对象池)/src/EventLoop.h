// EventLoop.h

#pragma once

#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <functional>
#include <queue>
#include <unordered_set>

class Channel;
class Epoll;
class ClientSocket;
class TcpServer;
class EventLoop;
class TcpConnection;
class TcpClient;

class EventLoop
{
public:
    using MessageCallBack = std::function<void(const std::shared_ptr<TcpConnection>&, std::string&)>;

    EventLoop();

    ~EventLoop();

public:
    void InitServer(TcpServer* tcpServer) { tcpServer_ = tcpServer; }

    // 启动事件循环
    void Loop();

    // 停止事件循环
    void Quit();

    // 添加或更新channel(线程安全)
    void AddChannel(std::unique_ptr<Channel> channel, std::string strInfo = "OtherError");

    // 钥匙类：只有 TcpClient 能构造
    /*
    class RemoveChannelNowToken {
    private:
        RemoveChannelNowToken() = default;          // 私有构造
        friend class TcpClient;           // 仅 TcpClient 可访问构造
    };

    bool NowToRemoveChannel(int fd, RemoveChannelNowToken token);
    */

    void DelayRemoveQueue(int fd, bool bTcpClient = false, std::function<void()> = nullptr);

    void AddEventToUpdateChannel(int fd, int event);

    void DelEventToUpdateChannel(int fd, int event);

    bool CheckEvetFromChannel(int fd, int event);

    // 将connection_list移植到event_loop中来
    void HandleNewConnection(std::shared_ptr<TcpConnection> newConnection);

    void ClosedConnection(const std::shared_ptr<TcpConnection>& conn);

    bool StartConnIdleTimer();

public:
    // 跨线程调度: 如果当前是IO线程则直接执行，否则放入队列
    void RunInLoop(std::function<void()> cb);
    void QueueInLoop(std::function<void()> cb);
    void AssertInLoopThread(std::string strInfo);

    std::thread::id GetThreadId() const { return threadId_; }
private:
     bool IsInLoopThread() const;

     // 移除Channel
     void RemoveServerChannelInLoop(int fd);

     using PairTcpClient = std::pair<int, std::function<void()>>;
     void ClientChannelRemoveInLoop(const PairTcpClient& pairTcpClient);

     void DoPendingFunctors();
     void WakeUp();              // 用于唤醒epoll_wait

// 新增定时器
public:
    // 单次定时器 delay为delay毫秒执行回调
    void RunAfter(uint64_t timer_id, std::chrono::milliseconds delay, std::function<void()> funcCb);

    // 周期性回调
    void RunEvery(uint64_t timer_id, std::chrono::milliseconds interval, std::function<void()> funcCb);

    uint64_t GenerateNewTimerId();

    void CancelTimer(uint64_t timer_id);

private:
    void HandleTimerRead();
    void UpdateTimerFd();
    void ExecuteExpiredTimers();

    
private:
    // 将connection_list移植到event_loop中来
    void RemoveConnectionByFd(int fd);

    void CheckIdleConnections(int idleSecTimeOut);

private:
    std::unique_ptr<Epoll>  epoll_;
    std::map<int, std::unique_ptr<Channel>> channels_;
    bool quit_;
    std::thread::id threadId_;
    std::vector<int> delayServerChannelsToRemove_;
    std::vector<PairTcpClient> delayRemoveForClientChannel_;

    std::vector<std::function<void()>> pendingFunctors_;
	std::mutex mutex_;

    int wakeUpFd_;
    //std::unique_ptr<Channel> wakeUpChannel_; 融入到channels_列表中去了，统一管理

    TcpServer* tcpServer_; //保存裸指针
    
    int timerFd_;
    //std::unique_ptr<Channel> timerChannel_; 融入到channels_列表中去了，统一管理
    // 存储所有定时器：按超时时间排序（multimap 按时间自动排序）
    //std::multimap<std::chrono::steady_clock::time_point, std::function<void()>> timersFunc_;
    // 优化定时器，用小顶堆来实现，提高插入和删除的效率
    
    struct TimerNode
    {
        std::chrono::steady_clock::time_point expire_time;
        std::function<void()> callback;
        uint64_t id;            // 唯一的定时器id,用于取消定时器

        // 默认大顶堆，则取反得到小顶堆
        bool operator<(const TimerNode& other_timer) const
        {
            if(expire_time > other_timer.expire_time){
                return true;
            }else if(expire_time == other_timer.expire_time){
                return id > other_timer.id;
            }

            return false;
        }
        
    };

    std::priority_queue<TimerNode> timersFunc_; 
    std::unordered_set<uint64_t>  cancel_timer_list_;
    std::atomic<uint64_t> next_timer_id_{1};

    // 将connection_list一直到对应的eventloop中来
    std::map<int, std::shared_ptr<TcpConnection>> mapTcpConnection_;
    

   // int idleTimeOutSecs_ = 60; // 连接空闲时间断开，默认是60秒(<=0 则空闲时间可以无限)
};