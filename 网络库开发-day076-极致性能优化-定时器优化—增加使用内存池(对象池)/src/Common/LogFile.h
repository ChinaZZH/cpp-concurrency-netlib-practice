#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <map>

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
    LogFile() = default;
    
    // 静态成员实例（在类外定义并初始化）
    static LogFile instance;

    
private:
    std::map<std::string, std::ofstream>  log_file_system_;
    std::mutex mtx_;
};

