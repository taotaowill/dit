#include "dit_server_impl.h"
#include "proto/dit.pb.h"
#include "utils/common_util.h"

namespace baidu {
namespace dit {

DitServerImpl::DitServerImpl() {
}

DitServerImpl::~DitServerImpl() {
}

void DitServerImpl::Share(::google::protobuf::RpcController* controller,
                          const proto::ShareRequest* request,
                          proto::ShareResponse* response,
                          ::google::protobuf::Closure* done) {

    MutexLock lock(&mutex_);
    if (request->has_path()) {
        response->mutable_return_code()->set_status(proto::kError);
        response->mutable_return_code()->set_message("path should not be empty");
        done->Run();
    }

    std::string path = request->path();
    std::string key = generate_uuid();
    paths_[key] = path;
    response->mutable_return_code()->set_status(proto::kOk);
    response->set_key(key);
    LOG(INFO) << "share ok, path: " << path << ", key: " << key;
    done->Run();
}

void DitServerImpl::List(::google::protobuf::RpcController* controller,
                         const proto::ListRequest* request,
                         proto::ListResponse* response,
                         ::google::protobuf::Closure* done) {
    MutexLock lock(&mutex_);
    LOG(INFO) << "list";
    done->Run();
}

void DitServerImpl::Download(::google::protobuf::RpcController* controller,
                             const proto::DownloadRequest* request,
                             proto::DownloadResponse* response,
                             ::google::protobuf::Closure* done) {
    LOG(INFO) << "download";
    done->Run();
}

}
}
