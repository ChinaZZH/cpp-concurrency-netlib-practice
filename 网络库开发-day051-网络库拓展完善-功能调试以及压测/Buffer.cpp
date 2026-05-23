#include "Buffer.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/uio.h>
#include <errno.h>

Buffer::Buffer(size_t initialSize /*= kInitialSize*/)
:buffer_(initialSize, 0)
,read_index_(0)
,write_index_(0)
{

}

void Buffer::Swap(Buffer& rhs)
{
    if(this != &rhs)
    {
        buffer_.swap(rhs.buffer_);
        std::swap(read_index_, rhs.read_index_);
        std::swap(write_index_, rhs.write_index_);
    }
}


// 读取解析后，移动readIndex_
void Buffer::Retrieve(size_t len)
{
    size_t readable_len = ReadableBytes();
    if(len < readable_len)
    {
        read_index_ += len;
    }else{
        RetrieveAll();
    }
}


// 取出所有数据并返回字符串
std::string Buffer::RetrieveAllAsString()
{
    std::string strResult(Peek(), ReadableBytes());
    RetrieveAll();
    return strResult;
}


// 向buffer添加数据
void Buffer::Append(const char* data, size_t len)
{
    EnsureWritableBytes(len);
    std::copy(data, data + len, Begin() + write_index_);
    write_index_ += len;
}


// 预留writable 空间，确保能容纳len字节
void Buffer::EnsureWritableBytes(size_t len)
{
    if(WritableBytes() >= len)
    {
        return ;
    }

    MakeSpace(len);
}


void Buffer::MakeSpace(size_t len)
{
    if(PrependableBytes() + WritableBytes() >= len)
    {
        size_t readable_len = ReadableBytes();
        std::memmove(Begin(), Begin() + read_index_, readable_len);
        read_index_ = 0;
        write_index_ = readable_len;

    }else{
        size_t newSize = std::max(buffer_.size() * 2, buffer_.size() + len);
        buffer_.resize(newSize);
    }
}


// 在缓冲区头部预留空间(用于prepend)
void Buffer::Prepend(const void* data, size_t len)
{
    if(PrependableBytes() < len)
    {
        size_t readable_len = ReadableBytes();
        if(buffer_.size() < write_index_ + len)
        {
            buffer_.resize(write_index_ + len);
        }

        std::memmove(Begin() + read_index_ + len, Begin() + read_index_, readable_len);
        read_index_ += len;
        write_index_ += len;
    }

    read_index_ -= len;
    std::memcpy(Begin() + read_index_, data, len);
}

// 从fd读取数据(非阻塞)
ssize_t Buffer::ReadFd(int fd, int* nSaveErrno)
{
   char extrabuf[65536] = {0};
   struct iovec vec[2];
   const size_t writable = WritableBytes();

   vec[0].iov_base = Begin() + write_index_;
   vec[0].iov_len = writable;
   vec[1].iov_base = extrabuf;
   vec[1].iov_len = sizeof(extrabuf);

   const int iovcnt = (writable < sizeof(extrabuf))?2:1;
   ssize_t n = readv(fd, vec, iovcnt);
   if(n < 0){
        *nSaveErrno = errno;
   }else if(static_cast<size_t>(n) <= writable){
        write_index_ += n;
   }else{
        write_index_ =  buffer_.size();
        Append(extrabuf, n - writable);
   }

   return n;
}


// 发送数据到fd(非阻塞，尽量发送)
ssize_t Buffer::WriteFd(int fd, int* nSaveErrno)
{
    ssize_t n = ::write(fd, Peek(), ReadableBytes());
    if(n <= 0)
    {
        if(n < 0)
        {
            (*nSaveErrno) = errno;
        }
        
        return n;
    }

    Retrieve(n);
    return n;
}