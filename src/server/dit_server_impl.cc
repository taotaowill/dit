#include "dit_server_impl.h"

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
    done->Run();
}

void DitServerImpl::List(::google::protobuf::RpcController* controller,
                         const proto::ListRequest* request,
                         proto::ListResponse* response,
                         ::google::protobuf::Closure* done) {
    done->Run();
}

}
}
