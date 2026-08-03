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
    // (revents_ & EPOLLHUP) && (revents_ & EPOLLIN) 为服务端半关闭，但是客户端仍然发送读事件上来
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


     //bool bCloseHup =  (() && (revents_ & EPOLLIN))?true:false;
    if(revents_ & EPOLLHUP)
    {
        if(closeCallBack_)
        {
            closeCallBack_();
        }
    }


    // 客户端 ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) 连接断了并且没有可读事件
    //bool bErrorHup =  ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))?true:false;
    if(revents_ & EPOLLERR)
    {
        if(errorCallBack_)
        {
            //std::cout << "Channel::HandleEvent errorCallBack revents_:=" << revents_ << " EPOLLERR:="  <<  (revents_ & EPOLLERR) << " EPOLLHUP:= " << (revents_ & EPOLLHUP)  << std::endl;
            errorCallBack_();
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