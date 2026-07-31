#include "EventSink.h"
#include <iostream>
#include <sstream>

EventSink::EventSink()
{

}


EventSink::~EventSink()
{
    Shutdown();
}


// 初始化：指定存储路径，启动后台写入线程
void EventSink::Init(const std::string& file_path)
{
    file_path_ = file_path;
    output_file_.open(file_path, std::ios::out | std::ios::binary | std::ios::app);
    if(!output_file_.is_open())
    {
        std::cerr << "[EventSink] Failed to open: " << file_path_ << std::endl;
        return;
    }

    running_ = true;
    writer_thread_ = std::thread(&EventSink::WriteLoop, this);
    std::cout << "[EventSink] Initialized, writing to " << file_path_ << std::endl;
}


// 记录一个事件（线程安全，非阻塞）
void EventSink::Record(const GameEvent& event, RecordToken token)
{
    if(false == running_)
    {
        return ;
    }

    {
        std::lock_guard<std::mutex> lk(queue_mutex_);
        pending_queue_.push(event);
    }
    
    queue_cv_.notify_one();
}

// 关闭并等待所有事件写入完成
void EventSink::Shutdown()
{
    running_.store(false, std::memory_order_release);
    if(writer_thread_.joinable())
    {
        writer_thread_.join();
    }

    while(!pending_queue_.empty())
    {
        const GameEvent& event = pending_queue_.front();
        this->SerializeGameEventToFile(event);
        pending_queue_.pop();
    }

    if(output_file_.is_open())
    {
        output_file_.close();
    }

    std::cout << "[EventSink] Shutdown complete." << std::endl;
}

// 后台写入线程的主循环
void EventSink::WriteLoop()
{
    while(running_.load(std::memory_order_acquire))
    {
        std::queue<GameEvent> writeEvent;
        {
            std::unique_lock<std::mutex> lk(queue_mutex_);
            queue_cv_.wait(lk, [this](){
                return !pending_queue_.empty() || !running_;
            });

            writeEvent.swap(pending_queue_);
        }
        

        while(!writeEvent.empty())
        {
            const GameEvent& event = writeEvent.front();
            this->SerializeGameEventToFile(event);
            writeEvent.pop();
        }
    }
}


void EventSink::SerializeGameEventToFile(const GameEvent& event)
{
    // 序列化并写入文件（每个事件用 4 字节长度头 + 二进制数据）
    std::string strData;
    if(false == event.SerializeToString(&strData))
    {
        std::cerr << "[EventSink] Serialization failed." << std::endl;
        return;
    }

    // 写入长度头 + 数据(便于后续解析)
    uint32_t size = static_cast<uint32_t>(strData.size());
    output_file_.write(reinterpret_cast<const char*>(&size), sizeof(size));
    output_file_.write(strData.data(), strData.size());
    output_file_.flush();
}