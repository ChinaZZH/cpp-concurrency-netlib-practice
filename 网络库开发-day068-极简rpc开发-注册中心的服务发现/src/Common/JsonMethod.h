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

    std::string add(const std::string& params);
    std::string echo(const std::string& params);
    std::string login(const std::string& params);
}
