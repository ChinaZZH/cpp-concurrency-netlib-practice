#include "Epoll.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include "Channel.h"
#include <cerrno>

Epoll::Epoll()
:fd_(-1)
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

    memset(events_list_, 0, sizeof(events_list_));
}


Epoll::Epoll(Epoll&& other) noexcept
:fd_(other.fd_)
{
    memcpy(events_list_, other.events_list_, sizeof(events_list_));

    other.fd_ = -1;
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
        memcpy(events_list_, other.events_list_, sizeof(events_list_));

        other.fd_ = -1;
        memset(other.events_list_, 0, sizeof(other.events_list_));
    }

    return *this;
}



bool Epoll::AddFd(Channel* ptrChannel)
{
    if(!IsValid() || !ptrChannel)
    {
        std::cerr << "Epoll::AddFd error \n";
        return false;
    }

    struct epoll_event ev;
    ev.events = ptrChannel->GetEvents();
    ev.data.ptr = ptrChannel;
    if(epoll_ctl(fd_, EPOLL_CTL_ADD, ptrChannel->GetFd(), &ev) < 0)
    {
        return false;
    }

    return true;
}


bool Epoll::ModifyFd(Channel* ptrChannel)
{
    if(!IsValid() || !ptrChannel)
    {
        std::cerr << "Epoll::ModifyFd error \n";
        return false;
    }

    struct epoll_event ev;
    ev.events = ptrChannel->GetEvents();
    ev.data.ptr = ptrChannel;
    return (0 == epoll_ctl(fd_, EPOLL_CTL_MOD, ptrChannel->GetFd(), &ev));
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
    
    std::cerr << "Epoll::RemoveFd epoll_ctl del Fail ErrorCode=" << errno << std::endl;
    return false;
}


int Epoll::Wait(std::vector<Channel*>& vecChannel)
{
    if(!IsValid())
    {
        std::cerr << "Epoll::Wait error \n" ;
        return -1;
    }

    int nfds = epoll_wait(fd_, events_list_, MAX_EVENTS, -1);
    if(nfds < 0)
    {
        std::cerr << "Epoll::Wait epoll_wait error \n" ;
        return -1;
    }

    for(int i = 0; i < nfds; ++i)
    {
        Channel* ch = static_cast<Channel*>(events_list_[i].data.ptr);
        if(ch)
        {
            ch->SetRevents(events_list_[i].events);
            vecChannel.push_back(ch);
        }
    }

    return nfds;
}

