#include "RpcLogFile.h"

RpcLogFile RpcLogFile::instance;


void RpcLogFile::Release()
{
    if(log_file_.is_open())
    {
        log_file_.close();
    }
}

bool RpcLogFile::OpenFile(std::string fileName)
{
    if(log_file_.is_open())
    {
        return false;
    }

    log_file_.open(fileName.c_str(), std::ios::app);
    return log_file_.is_open();
}

bool RpcLogFile::AppendContent(const std::string& context)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if(!log_file_.is_open())
    {
        return false;
    }

    log_file_ << context << std::endl;
    return true;
}

