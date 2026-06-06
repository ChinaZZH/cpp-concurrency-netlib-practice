#pragma once


#include <iostream>
#include <chrono>
#include <iostream>

#include <signal.h>
#include <thread>
#include <nlohmann/json.hpp>




namespace JsonMethodLib
{

    using json = nlohmann::json;
    struct AddRequest{
        int a;
        int b;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AddRequest, a, b);

    /*
    void to_json(nlohmann::json& j, const AddRequest& req)
    {
        j = nlohmann::json{{"a", req.a} , {"b", req.b}};
    }
    */

    struct AddResponse{
        int result;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AddResponse, result);

    /*
    void from_json(const nlohmann::json& j, AddResponse& resp)
    {
        j.at("result").get_to(resp.result);
    }
    */


    struct LoginRequest{
        std::string user;
        std::string pass;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LoginRequest, user, pass);


    struct LoginResponse{
        int code;
        std::string token;
        std::string msg;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LoginResponse, code, token, msg);


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
