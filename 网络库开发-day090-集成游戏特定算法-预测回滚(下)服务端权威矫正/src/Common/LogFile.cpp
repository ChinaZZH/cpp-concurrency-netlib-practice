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

    for(auto& itr:log_file_system_)
    {
        auto& logStruct = (itr.second);
        std::lock_guard<std::mutex> lock(logStruct.second);
        if(logStruct.first.is_open())
        {
            logStruct.first.close();
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

    
    LogStructData* ptrLogStruct;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto& logStructData = log_file_system_[fileName];
        if(!logStructData.first.is_open())
        {
            logStructData.first.open(fileName.c_str(), std::ios::app);
        }

        ptrLogStruct = &logStructData;
    }
    
    if(ptrLogStruct)
    {
        std::lock_guard<std::mutex> lock(ptrLogStruct->second);
        (ptrLogStruct->first) << context << std::endl;
    }

    return true;
}