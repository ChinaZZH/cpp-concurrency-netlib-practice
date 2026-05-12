#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <cstring>

class Buffer
{
public:
    static const size_t kInitialSize = 4096;

    Buffer(size_t initialSize = kInitialSize);

    void Swap(Buffer& rhs);

    // 可读数据长度
    size_t ReadableBytes() const { return write_index_ - read_index_; }
    
    size_t WritableBytes() const { return buffer_.size() - write_index_; } 

    size_t PrependableBytes() const { return read_index_; }

    // 去可读数据的其实地址
    const char* Peek() const { return Begin() + read_index_; }

    char* Peek() { return Begin() + read_index_; }

    // 读取解析后，移动readIndex_
    void Retrieve(size_t len);

    void RetrieveAll()  { read_index_ = write_index_ = 0; }

    // 取出所有数据并返回字符串
    std::string RetrieveAllAsString();

    // 向buffer添加数据
    void Append(const char* data, size_t len);
    
    void Append(const std::string& str) {  Append(str.c_str(), str.length()); }

    // 从fd读取数据(非阻塞)
    ssize_t ReadFd(int fd, int* nSaveErrno);

    // 发送数据到fd(非阻塞，尽量发送)
    ssize_t WriteFd(int fd, int* nSaveErrno);

    // 在缓冲区头部预留空间(用于prepend)
    void Prepend(const void* data, size_t len);

    // 预留writable 空间，确保能容纳len字节
    void EnsureWritableBytes(size_t len);

private:
    char* Begin() { return buffer_.data(); }

    const char* Begin() const  { return buffer_.data();  }

    void MakeSpace(size_t len);

private:
    std::vector<char> buffer_;
    size_t  read_index_;
    size_t  write_index_;
};
