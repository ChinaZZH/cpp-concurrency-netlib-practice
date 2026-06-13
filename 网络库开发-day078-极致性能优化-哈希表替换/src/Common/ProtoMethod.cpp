#include "ProtoMethod.h"
#include "../../build/proto_gen/add.pb.h"
#include "../../build/proto_gen/rpc.pb.h"
#include "../../build/proto_gen/login.pb.h"

namespace ProtoMethod
{
    std::string add(const std::string& params)
    {
        AddRequest req;
        if(!req.ParseFromString(params))
        {
            throw std::runtime_error("parse AddRequest failed");
        }

        AddResponse result;
        result.set_sum(req.a() + req.b());

        std::string strResult;
        result.SerializeToString(&strResult);
        return strResult;
    }
}