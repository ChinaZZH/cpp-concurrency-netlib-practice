
#include "AuditLog.h"
#include <sstream>
#include <iostream>
#include <iomanip>


AuditLogger::AuditLogger()
{

}

AuditLogger::~AuditLogger()
{
    Shutdown();
}

// 初始化（指定日志文件路径）
void AuditLogger::Init(const std::string& log_path, bool async_mode /*= true*/)
{
    log_path_ = log_path;
    async_mode_ = async_mode;
    log_file_.open(log_path_, std::ios::out | std::ios::app);
    if(false == log_file_.is_open())
    {
        std::cerr << "[AuditLogger] Failed to open log file: " << log_path_ << std::endl;
        return ;
    }

    if(async_mode_)
    {
        running_ = true;
        worker_thread_ = std::thread(&AuditLogger::FlushWorker, this);
    }

    std::cout << "[AuditLogger] Initialized, writing to " << log_path_ << std::endl;
}

// 记录一条审计日志（线程安全）
void AuditLogger::Log(const AuditEntry& entry)
{
    if(async_mode_)
    {
        {
            std::lock_guard<std::mutex> lk(queue_mutex_);
            pending_queue_.push(std::move(entry));
        }

        queue_cv_.notify_one();
    }
    else{
        this->WriteEntry(entry);
    }
}

// 关闭日志系统（等待队列中的日志写完）
void AuditLogger::Shutdown()
{
    if(async_mode_)
    {
        running_.store(false, std::memory_order_release);
        if(worker_thread_.joinable())
        {
            worker_thread_.join();
        }
    }
    
    
    if(log_file_.is_open())
    {
        log_file_.close();
    }
}


// 异步写入线程的主循环
void AuditLogger::FlushWorker()
{
    while(running_.load(std::memory_order_acquire))
    {
        std::unique_lock<std::mutex> lk(queue_mutex_);
        queue_cv_.wait(lk, [this](){ return !pending_queue_.empty() || !running_; });

        while(!pending_queue_.empty())
        {
            AuditEntry entry = std::move(pending_queue_.front());
            pending_queue_.pop();
            lk.unlock();
            WriteEntry(entry);
            lk.lock();
        }   
    }
}


void AuditLogger::WriteEntry(const AuditEntry& entry)
{
    if(false == log_file_.is_open())
    {
        return;
    }

    std::ostringstream line;
    line << entry.timestamp_ms << ",";
    line << entry.player_id << ",";
    line << entry.violation_type << ",";
    line << entry.current_value << ",";
    line << entry.threshold_value << ",";
    line << entry.violation_count << ",";
    line << entry.penalty_action << "\n";

    log_file_ << line.str();
    log_file_.flush();
}


