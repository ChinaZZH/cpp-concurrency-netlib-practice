#include "Epoll.h"
#include <iostream>
#include <cstring>
#include <unistd.h>

Epoll::Epoll()
:fd_(-1)
,event_count_(0)
{
    fd_ = epoll_create1(0);
    if(fd_ < 0)
    {
        std::cerr << "epoll_create error \n";
    }
}


Epoll::~Epoll()
{
    if(fd_ >= 0)
    {
        close(fd_);
        fd_ = -1;
    }

    event_count_ = 0;
    memset(events_list_, 0, sizeof(events_list_));
}


Epoll::Epoll(Epoll&& other) noexcept
:fd_(other.fd_)
,event_count_(other.event_count_)
{
    memcpy(events_list_, other.events_list_, sizeof(events_list_));

    other.fd_ = -1;
    other.event_count_ = 0;
    memset(other.events_list_, 0, sizeof(other.events_list_));
}


Epoll& Epoll::operator=(Epoll&& other) noexcept
{
    if(this != &other)
    {
        if(fd_ >= 0)
        {
            close(fd_);
        }

        fd_ = other.fd_;
        event_count_ = other.event_count_;
        memcpy(events_list_, other.events_list_, sizeof(events_list_));

        other.fd_ = -1;
        other.event_count_ = 0;
        memset(other.events_list_, 0, sizeof(other.events_list_));
    }

    return *this;
}



bool Epoll::AddFd(int nSocketFd, uint32_t events /*= EPOLLIN*/)
{
    if(nSocketFd < 0 || !IsValid())
    {
        std::cerr << "Epoll::AddFd error \n";
        return false;
    }

    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = nSocketFd;
    if(epoll_ctl(fd_, EPOLL_CTL_ADD, nSocketFd, &ev) < 0)
    {
        return false;
    }

    return true;
}


bool Epoll::ModifyFd(int nSocketFd, uint32_t events)
{
    if(nSocketFd < 0 || !IsValid())
    {
        std::cerr << "Epoll::ModifyFd error \n";
        return false;
    }

    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = nSocketFd;
    return (0 == epoll_ctl(fd_, EPOLL_CTL_MOD, nSocketFd, &ev));
}

bool Epoll::RemoveFd(int nSocketFd)
{
    if(nSocketFd < 0 || !IsValid())
    {
        std::cerr << "Epoll::RemoveFd error \n";
        return false;
    }

    int result = epoll_ctl(fd_, EPOLL_CTL_DEL, nSocketFd, nullptr);
    if(0 == result)
    {
        std::cout << "close connection client socket fd="<< nSocketFd << std::endl;
        return true;
    }
    
    std::cerr << "Epoll::RemoveFd epoll_ctl del fail \n";
    return false;
}


int Epoll::Wait()
{
    if(!IsValid())
    {
        std::cerr << "Epoll::Wait error \n" ;
        return 0;
    }

    event_count_ = epoll_wait(fd_, events_list_, MAX_EVENTS, -1);
    return event_count_;
}


int Epoll::GetClientFd(int index)
{
    if(index >= event_count_ || index < 0)
    {
        return -1;
    }


    return events_list_[index].data.fd;
}