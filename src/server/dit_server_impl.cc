#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "dit_server_impl.h"
#include "proto/dit.pb.h"
#include "utils/common_util.h"

DECLARE_string(nexus_addr);
DECLARE_string(nexus_root);
DECLARE_string(server_nexus_prefix);

namespace baidu {
namespace dit {

DitServerImpl::DitServerImpl() {
    nexus_ = new InsSDK(FLAGS_nexus_addr);
}

DitServerImpl::~DitServerImpl() {
    delete nexus_;
}

bool DitServerImpl::RegisterOnNexus(const std::string& endpoint) {
    SDKError err;
    bool ret = nexus_->Lock(FLAGS_nexus_root + FLAGS_server_nexus_prefix + "/" + endpoint, &err);
    if (!ret) {
        LOG(WARNING) << "failed to acquire nexus lock, " << err;
        return false;
    }
    return true;
}

void DitServerImpl::Ls(::google::protobuf::RpcController* controller,
                       const proto::LsRequest* request,
                       proto::LsResponse* response,
                       ::google::protobuf::Closure* done) {
    LOG(INFO) << "ls path: " << request->path();
    if (!request->has_path()) {
        response->mutable_ret()->set_status(proto::kError);
        response->mutable_ret()->set_message("invalid param, path is required");
        done->Run();
        return;
    }
    std::string path = request->path();
    if (!boost::filesystem::exists(path)) {
        response->mutable_ret()->set_status(proto::kError);
        response->mutable_ret()->set_message("path: " + path + " does not exist");
        done->Run();
        return;
    }

    bool optAll = false;
    for (int i=0; i<request->options_size(); i++) {
        if (proto::kAll == request->options(i)) {
            optAll = true;
            break;
        }
    }

    if(boost::filesystem::is_directory(request->path())) {
        for (boost::filesystem::directory_iterator end_dir_it, it(request->path()); it != end_dir_it; ++it) {
            std::string p = it->path().string();
            std::string relative_path = p;
            boost::replace_first(relative_path, path, "");
            if (!optAll && (
                    boost::algorithm::starts_with(relative_path, ".")
                    || boost::algorithm::starts_with(relative_path, "/."))) {
                continue;
            }
            proto::DitFile* file = response->add_files();
            if (boost::filesystem::is_directory(p)) {
                file->set_type(proto::kDitDirectory);
                if(!boost::algorithm::ends_with(p, "/")) {
                    p += "/";
                }
            } else {
                file->set_type(proto::kDitFile);
            }
            file->set_name(p);
        }
    } else {
        proto::DitFile* file = response->add_files();
        file->set_name(path);
        file->set_type(proto::kDitFile);
    }
    response->mutable_ret()->set_status(proto::kOk);
    done->Run();

    return;
}

}
}
