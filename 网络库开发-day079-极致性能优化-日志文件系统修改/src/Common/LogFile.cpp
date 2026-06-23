#include "LogFile.h"

LogFile LogFile::instance;

LogFile::LogFile()
{
    int thread_count = 5;
    log_thread_pool_.reserve(thread_count);
    for(int i = 0; i < thread_count; ++i){
        log_thread_pool_.emplace_back([this](){
            while(true)
            {
                std::pair<std::string, std::string> logData;
                log_content_task_.wait_dequeue(logData);
                if(!logData.first.empty()){
                    this->HelpToAppendLog(logData.first, logData.second);
                }else if(stop_flag_){
                    break;
                }else{
                    std::this_thread::yield();
                }
            }
        });
    }
}


LogFile::~LogFile()
{
    Release();
}


void LogFile::Release()
{
    {
        stop_flag_ = true;
    }

    for(auto& th : log_thread_pool_)
    {
        if(th.joinable())
        {
            th.join();
        }
    }

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
    if(stop_flag_)
    {
        return false;
    }
    
    log_content_task_.enqueue(std::pair(fileName, context));
    return true;
}


bool LogFile::HelpToAppendLog(const std::string& fileName, const std::string& context)
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