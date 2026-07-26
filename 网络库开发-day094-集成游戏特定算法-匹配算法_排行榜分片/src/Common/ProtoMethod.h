#pragma once

#include <string>
#include <memory>

class TcpConnection;
namespace ProtoMethod
{
    std::string add(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& params);
}