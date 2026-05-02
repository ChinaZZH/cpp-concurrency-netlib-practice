#pragma once


#include <string>
#include <sys/epoll.h>
#include <memory>
#include <vector>

#define MAX_EVENTS 1024

class Channel;
class Epoll
{
public:
    Epoll();

    ~Epoll();
    
public:
    Epoll(const Epoll& other)                = delete;
    Epoll& operator=(const Epoll& other)     = delete;

    Epoll(Epoll&& other)              noexcept;
    Epoll& operator=(Epoll&& other)   noexcept;

public:
    bool IsValid() const { return fd_ >= 0; }

    bool AddFd(Channel* ptrChannel);

    bool ModifyFd(Channel* ptrChannel);

    bool RemoveFd(int nSocketFd);

    int  Wait(std::vector<Channel*>& vecChannel);

  
private:
    int fd_ = -1;
    struct epoll_event events_list_[MAX_EVENTS];
};
