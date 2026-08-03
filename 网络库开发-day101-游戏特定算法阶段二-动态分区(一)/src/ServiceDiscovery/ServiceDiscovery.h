#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <thread>
#include <memory>
#include <atomic>
#include <mutex>
#include <vector>



struct EndPoint
{
    std::string ip;
    int port;

    bool operator==(const EndPoint& point) const
    {
        return (ip == point.ip && port == point.port);
    }
};


namespace std {
    template<> struct hash<EndPoint> {
        size_t operator() (const EndPoint& point) const {
            return (hash<std::string>()(point.ip)) ^ (hash<int>()(point.port) << 1);
        }
    };
}


class ServiceDiscovery
{
public:
    using DiscoveryCallback = std::function<void(const std::vector<EndPoint>&)>;

    ServiceDiscovery(const std::string& registry_host, int registry_port, const std::string& service_name);

    ~ServiceDiscovery();

    void Start(DiscoveryCallback cb, int refresh_interval_sec = 30);

private:
    void RefreshLoop();
    
private:
    std::string registry_host_;
    int registry_port_;
    std::string service_name_;

    DiscoveryCallback callback_;
    int refresh_interval_sec_;

    std::unique_ptr<std::thread> thread_;
    std::atomic<bool> stop_ = false;
};