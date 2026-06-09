#include "JsonMethod.h"


namespace JsonMethodLib
{
    std::string add(const std::string& params)
    {
        //std::this_thread::sleep_for(std::chrono::seconds(2));
        int result = 0;
        try
        {
            // std::cout << "start add params=" << params << std::endl;
            auto j = json::parse(params);
            int a =  j["a"];
            int b = j["b"];
            result = a + b;
            // std::cout << "add result :=" << result << std::endl;
        }
        catch(const std::exception& e)
        {
            throw std::runtime_error("call add function error");
        }
        
        json res = {{"result", result}};
        return res.dump();
    }


    std::string echo(const std::string& params)
    {
        return params;
    }

    std::string login(const std::string& params)
    {
        try
        {
            auto j = json::parse(params);
            if("admin" == j["user"] && "123" == j["pass"])
            {
                return R"({"code":0,"token":"abc123", "msg":"auth successed"})";
            }else{
                return R"({"code":1,"token":"", "msg":"auth failed"})";
            }
        }
        catch(const std::exception& e)
        {
            throw std::runtime_error("call login function error");
        }

        return R"({"code":1,"token":"", "msg":"auth failed"})";
    }
}