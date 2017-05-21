#pragma once
#include <map>
#include <string>

#include <common/thread_pool.h>
#include <glog/logging.h>
#include <gflags/gflags.h>

#include "proto/dit.pb.h"

namespace baidu {
namespace dit {

class DitServerImpl: public proto::DitServer {
public:
    DitServerImpl();
    ~DitServerImpl();
    void Share(::google::protobuf::RpcController* controller,
               const proto::ShareRequest* request,
               proto::ShareResponse* response,
               ::google::protobuf::Closure* done);
    void List(::google::protobuf::RpcController* controller,
               const proto::ListRequest* request,
               proto::ListResponse* response,
               ::google::protobuf::Closure* done);
private:
    std::map<std::string, std::string> paths;
    ThreadPool background_pool;
};

}
}
