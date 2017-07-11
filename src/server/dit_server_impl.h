#pragma once
#include <map>
#include <string>

#include <common/thread_pool.h>
#include <common/mutex.h>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <ins_sdk.h>

#include "proto/dit.pb.h"

namespace baidu {
namespace dit {

typedef ::galaxy::ins::sdk::InsSDK InsSDK;
typedef ::galaxy::ins::sdk::SDKError SDKError;

class DitServerImpl: public proto::DitServer {
public:
    DitServerImpl();
    ~DitServerImpl();
    void Ls(::google::protobuf::RpcController* controller,
            const proto::LsRequest* request,
            proto::LsResponse* response,
            ::google::protobuf::Closure* done);
    bool RegisterOnNexus(const std::string& endpoint);
private:
    bool Init();
private:
    ThreadPool background_pool_;
    Mutex mutex_;
    InsSDK* nexus_;
    std::string endpoint_;
};

}
}
