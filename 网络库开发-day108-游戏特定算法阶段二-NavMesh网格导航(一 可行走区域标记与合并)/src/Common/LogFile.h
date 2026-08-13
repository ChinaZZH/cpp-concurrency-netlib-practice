#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <map>
#include <vector>
#include <thread>
#include <absl/container/flat_hash_map.h>
#include "BlockingConcurrentQueue.h"

// 主要排查问题用，后期如果要用，需要异步写日志。
class LogFile
{
public:
    // 获取单例实例（返回引用）
    static LogFile& getInstance() {
        return instance;
    }

    // 禁止拷贝和赋值
    LogFile(const LogFile&) = delete;
    LogFile& operator=(const LogFile&) = delete;

    ~LogFile();

    //bool OpenFile(std::string fileName);

    bool AppendContent(const std::string& fileName, const std::string& context);

public:
    void Release();

private:
    // 私有构造函数
    LogFile();
    
    // 静态成员实例（在类外定义并初始化）
    static LogFile instance;

    bool HelpToAppendLog(const std::string& fileName, const std::string& context);
    
private:
    std::vector<std::thread> log_thread_pool_;
    moodycamel::BlockingConcurrentQueue<std::pair<std::string, std::string>> log_content_task_;
    std::atomic<bool>  stop_flag_ = false;

    // 给每个文件配一把小锁
    using LogStructData = std::pair<std::ofstream, std::mutex>; 
    std::map<std::string, LogStructData> log_file_system_;
    std::mutex mtx_;
};

