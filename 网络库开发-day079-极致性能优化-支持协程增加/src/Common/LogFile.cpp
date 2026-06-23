#include "LogFile.h"

LogFile LogFile::instance;

LogFile::~LogFile()
{
    Release();
}


void LogFile::Release()
{
    std::lock_guard<std::mutex> lock(mtx_);
    for(auto& itr:log_file_system_)
    {
        auto& logfile = (itr.second);
        if(logfile.is_open())
        {
            logfile.close();
        }
    }
}


bool LogFile::AppendContent(const std::string& fileName, const std::string& context)
{
    if(fileName.empty())
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(mtx_);
    auto& logfile = log_file_system_[fileName];
    if(!logfile.is_open())
    {
        logfile.open(fileName.c_str(), std::ios::app);
    }


    logfile << context << std::endl;
    return true;
}

