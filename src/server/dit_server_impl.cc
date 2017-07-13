#include <iostream>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

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

void DitServerImpl::GetFileMeta(::google::protobuf::RpcController* controller,
                                const proto::GetFileMetaRequest* request,
                                proto::GetFileMetaResponse* response,
                                ::google::protobuf::Closure* done) {
    LOG(INFO) << "get file meta, path: " << request->path();
    if (!request->has_path()) {
        response->mutable_ret()->set_status(proto::kError);
        response->mutable_ret()->set_message("invalid param, path is required");
        done->Run();
        return;
    }

    boost::filesystem::path path(request->path());
    if (!boost::filesystem::exists(path)) {
        response->mutable_ret()->set_status(proto::kError);
        response->mutable_ret()->set_message("path: " + path.string() + " does not exist");
        done->Run();
        return;
    }

    // file
    if (boost::filesystem::is_regular_file(path)) {
        proto::DitFileMeta* dit_file = response->add_files();
        dit_file->set_type(proto::kDitFile);
        uintmax_t file_size = boost::filesystem::file_size(path);
        dit_file->set_size(file_size);
        done->Run();
    }

    // directory
    if (boost::filesystem::is_directory(path)) {
        // parse options
        bool opt_r = false;
        bool opt_a = false;
        for (int i=0; i<request->options_size(); i++) {
            if (proto::kOptionAll == request->options(i)) {
                opt_a = true;
            }
            if (proto::kOptionRecursive == request->options(i)) {
                opt_r = true;
            }
        }
        if (opt_r) {
            for (boost::filesystem::recursive_directory_iterator end_dir_it, it(path); it!=end_dir_it; ++it) {
                try {
                    proto::DitFileMeta* dit_file = response->add_files();
                    if (boost::filesystem::is_directory(*it)){
                        dit_file->set_type(proto::kDitDirectory);
                        dit_file->set_size(0);
                    }
                    if(boost::filesystem::is_regular_file(*it)) {
                        // TODO opt_a
                        dit_file->set_type(proto::kDitFile);
                        uintmax_t file_size = boost::filesystem::file_size(*it);
                        dit_file->set_size(file_size);
                    }
                    dit_file->set_path(it->path().string());
                } catch (const std::exception& ex ){
                    LOG(WARNING) << ex.what();
                    continue;
                }
            }
        } else {
            for (boost::filesystem::directory_iterator end_dir_it, it(path); it != end_dir_it; ++it) {
                try {
                    proto::DitFileMeta* dit_file = response->add_files();
                    if (boost::filesystem::is_directory(*it)){
                        dit_file->set_type(proto::kDitDirectory);
                        dit_file->set_size(0);
                    }
                    if(boost::filesystem::is_regular_file(*it)) {
                        // TODO opt_a
                        dit_file->set_type(proto::kDitFile);
                        uintmax_t file_size = boost::filesystem::file_size(*it);
                        dit_file->set_size(file_size);
                    }
                    dit_file->set_path(it->path().string());
                } catch (const std::exception& ex ){
                    LOG(WARNING) << ex.what();
                    continue;
                }
            }
        }
        done->Run();
    }

    return;
}

void DitServerImpl::GetFileBlock(::google::protobuf::RpcController* controller,
                                 const proto::GetFileBlockRequest* request,
                                 proto::GetFileBlockResponse* response,
                                 ::google::protobuf::Closure* done) {
    std::string path = request->block().path();
    int64_t offset = request->block().offset();
    int64_t length = request->block().length();
    LOG(INFO)
       << "+++ FileBlock, file: [" << path
       << "], offset: [" << offset
       << "], length: [" << length
       << "]";

    char * buf = new char[length];
    FILE* fp;
    if (NULL != (fp = fopen(path.c_str(), "rb"))) {
        int l = fread(buf, 1, length, fp);
        std::string content = std::string(buf, l);
        proto::DitFileBlock* block = response->mutable_block(); 
        block->set_path(path);
        block->set_offset(offset);
        block->set_length(l);
        block->set_content(content);
        response->mutable_ret()->set_status(proto::kOk);
        LOG(INFO)
            << "--- FileBlock, file: [" << path
            << "], offset: [" << offset
            << "], length: [" << l
            << "]";
    } else {
        response->mutable_ret()->set_status(proto::kError);
        response->mutable_ret()->set_message("open file failed");
    }

    done->Run();
}

}
}
