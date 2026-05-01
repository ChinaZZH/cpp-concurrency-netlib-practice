#pragma once


#include <string>
#include <sys/epoll.h>

#define MAX_EVENTS 1024

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

    bool AddFd(int nSocketFd, uint32_t events = EPOLLIN);

    bool ModifyFd(int nSocketFd, uint32_t events);

    bool RemoveFd(int nSocketFd);

    int  Wait();

    int GetClientFd(int index);

private:
    int fd_ = -1;
    int event_count_ = 0;
    struct epoll_event events_list_[MAX_EVENTS];
};
