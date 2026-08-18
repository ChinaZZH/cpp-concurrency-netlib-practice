#pragma once
#include <unordered_map>
#include <functional>
#include <vector>
#include <string>
#include <memory>
#include <absl/container/flat_hash_map.h>

class TcpConnection;
class HttpRouteParamHandler
{
public:
    using RouteParams = absl::flat_hash_map<std::string, std::string>;
    using RouteFunc = std::function<void(const std::shared_ptr<TcpConnection>&, const RouteParams&, bool KeepAlive)>;

    HttpRouteParamHandler();

    ~HttpRouteParamHandler();

public:

    void RegisterRoute(const std::string& pattern, RouteFunc handler);

    bool MatchRoute(const std::string& path, RouteParams& params, RouteFunc& handler);

private:
    struct RouteHandler
    {
        std::string pattern; // 例如 "/user/:id" 
        RouteFunc handler;
        std::vector<std::string> param_names;           // 从 pattern 中解析出的参数名列表
        std::vector<std::string> pattern_segments;      // 预拆分的段
        //std::string mask;   // 有:存1 没有的存0
    };

    std::vector<RouteHandler> route_handler_list_;
};