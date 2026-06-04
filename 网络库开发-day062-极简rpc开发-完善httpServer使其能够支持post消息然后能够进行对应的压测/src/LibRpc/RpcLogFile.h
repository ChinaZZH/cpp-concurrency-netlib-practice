#pragma once

#include <string>
#include <fstream>
#include <mutex>

class RpcLogFile
{
public:
    // 获取单例实例（返回引用）
    static RpcLogFile& getInstance() {
        return instance;
    }

    // 禁止拷贝和赋值
    RpcLogFile(const RpcLogFile&) = delete;
    RpcLogFile& operator=(const RpcLogFile&) = delete;

public:
    void Release();

    bool OpenFile(std::string fileName);

    bool AppendContent(const std::string& context);

private:
    // 私有构造函数
    RpcLogFile() = default;
    
    // 静态成员实例（在类外定义并初始化）
    static RpcLogFile instance;

    
private:
    std::ofstream log_file_;
    std::mutex mtx_;
};

