#include "Channel.h"
#include <sys/epoll.h>
#include <iostream>
#include <cerrno>

Channel::Channel(int fd)
:fd_(fd), events_(0), revents_(0)
{

}


void Channel::HandleEvent() const
{
    if(revents_ & (EPOLLERR | EPOLLHUP))
    {
        if(errorCallBack_)
        {
            errorCallBack_();
        }

        return;
    }
    
    
    if(revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
    {
        if(readCallBack_)
        {
            readCallBack_();
        }
    }
    
    if(revents_ & EPOLLOUT)
    {
        if(writeCallBack_)
        {
            writeCallBack_();
        }
    }
}


void Channel::EnableEvent(int et)
{
    events_ |= et;
}

void Channel::DisableEvent(int et)
{
    events_ &= (~et);
}