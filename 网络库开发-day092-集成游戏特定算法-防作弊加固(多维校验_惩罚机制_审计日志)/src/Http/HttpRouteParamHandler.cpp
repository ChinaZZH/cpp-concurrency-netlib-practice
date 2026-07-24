#include "HttpRouteParamHandler.h"
#include "HttpContext.h"
#include "HttpServer.h"
#include "../TcpConnection.h"
#include <sstream>

HttpRouteParamHandler::HttpRouteParamHandler()
{
    this->RegisterRoute("/user/:id", [](const std::shared_ptr<TcpConnection>& con, const RouteParams& param, bool bKeepAlive){
        std::string id = param.at("id");
        std::string body = "<h1>User ID:" + id + "</h1>";
        auto response = HttpContext::GenerateResponseBySolveRequest(bKeepAlive, 200, body);
        HttpServer::SyncResponseToClient(con, response.first, response.second, body, bKeepAlive, HttpServer::SyncResponseToken());
    });
}

HttpRouteParamHandler:: ~HttpRouteParamHandler() = default;


void HttpRouteParamHandler::RegisterRoute(const std::string& pattern, RouteFunc handler)
{
    RouteHandler router;
    router.pattern = pattern;
    router.handler = handler;
    router.param_names.clear();
    router.pattern_segments.clear();

    std::stringstream ss(pattern);
    std::string reg;
    while(std::getline(ss, reg, '/'))
    {
        router.pattern_segments.push_back(reg);
        if(!reg.empty() && ':'== reg[0])
        {
            router.param_names.push_back(reg.substr(1));
        }
    }

    route_handler_list_.push_back(router);
}


bool HttpRouteParamHandler::MatchRoute(const std::string& path, RouteParams& params, RouteFunc& handler)
{
    std::vector<std::string> path_segments;
    std::stringstream ss(path);
    std::string reg;
    while(std::getline(ss, reg, '/'))
    {
        path_segments.push_back(reg);

    }


    for(const auto& route : route_handler_list_)
    {
        if(path_segments.size() != route.pattern_segments.size())
        {
            continue;
        }

        params.clear();
        bool match = true;
        for(int i = 0; i < route.pattern_segments.size(); ++i)
        {
            const auto& pattern_seg = route.pattern_segments[i];
            if(pattern_seg.empty())
            {
                continue;
            }

            if(':' == pattern_seg[0])
            {
                params[pattern_seg.substr(1)] =  path_segments[i];
            }
            else if(pattern_seg != path_segments[i])
            {
                match = false;
                break;
            }
        }

        if(match)
        {
            handler = route.handler;
            return true;
        }
    }

    return false;
}