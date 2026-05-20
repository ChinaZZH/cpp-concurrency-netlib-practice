#pragma once

#include <functional>
#include <cstdint>
#include <sys/epoll.h>

class Channel
{
public:
    using EventCallback = std::function<void()>;

    explicit Channel(int fd);

    ~Channel() = default;
    
public:
    void HandleEvent() const;

    int GetFd() const                { return fd_;     }
    uint32_t GetEvents() const       { return events_; }
    void SetRevents(uint32_t revents) { revents_ = revents; }

    void SetReadCallBack(EventCallback cb) { readCallBack_ = cb; }    
    void SetWriteCallBack(EventCallback cb) { writeCallBack_ = cb; }  
    void SetErrorCallBack(EventCallback cb) { errorCallBack_ = cb; }  

    void EnableReading()  { events_ |= EPOLLIN;     }
    void DisableReading() { events_ &= (~EPOLLIN);  }
    void EnableWriting()  { events_ |= EPOLLOUT;    }
    void DisableWriting() { events_ &= (~EPOLLOUT); }
    void EnableET()       { events_ |= EPOLLET;     }        
    
    void EnableEvent(int et);
    void DisableEvent(int et);

private:
    int fd_;
    uint32_t events_;
    uint32_t revents_;

    EventCallback readCallBack_;
    EventCallback writeCallBack_;
    EventCallback errorCallBack_;
};
