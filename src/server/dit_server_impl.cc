#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>

#include <boost/any.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "dit_server_impl.h"
#include "proto/dit.pb.h"
#include "utils/common_util.h"

DECLARE_string(nexus_addr);
DECLARE_string(nexus_root);
DECLARE_string(nexus_server_prefix);
DECLARE_int32(server_thread_num);

namespace baidu {
namespace dit {

DitServerImpl::DitServerImpl() : pool_(FLAGS_server_thread_num) {
    nexus_ = new InsSDK(FLAGS_nexus_addr);
}

DitServerImpl::~DitServerImpl() {
    delete nexus_;
}

bool DitServerImpl::RegisterOnNexus(const std::string& endpoint) {
    SDKError err;
    bool ret = nexus_->Lock(FLAGS_nexus_root + FLAGS_nexus_server_prefix + "/" + endpoint, &err);
    if (!ret) {
        LOG(WARNING) << "failed to acquire nexus lock, " << err;
        return false;
    }
    return true;
}

template<class T>
void travel_files(T it, T end_dir_it, proto::GetFileMetaResponse* response, bool opt_a) {
    for (; it!=end_dir_it; ++it) {
        try {
            // skip hidden file or dir
            if (!opt_a && it->path().filename().string()[0] == '.') {
                continue;
            }

            if (boost::filesystem::is_directory(*it)) {
                proto::DitFileMeta* dit_file = response->add_files();
                dit_file->set_type(proto::kDitDirectory);
                dit_file->set_path(it->path().string() + "/");
                dit_file->set_size(4096);
                dit_file->set_perms(boost::filesystem::status(it->path()).permissions());
            }

            if(boost::filesystem::is_regular_file(*it)) {
                proto::DitFileMeta* dit_file = response->add_files();
                dit_file->set_type(proto::kDitFile);
                dit_file->set_path(it->path().string());
                dit_file->set_size(boost::filesystem::file_size(*it));
                dit_file->set_perms(boost::filesystem::status(it->path()).permissions());
            }
        } catch (const std::exception& ex ) {
            LOG(WARNING) << ex.what();
            continue;
        }
    }
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
        dit_file->set_perms(boost::filesystem::status(path).permissions());
        dit_file->set_path(path.string());
        dit_file->set_type(proto::kDitFile);
        dit_file->set_size(boost::filesystem::file_size(path));
        done->Run();
        return;
    }

    // symblink
    if (boost::filesystem::is_symlink(path)) {
        proto::DitFileMeta* dit_file = response->add_files();
        dit_file->set_type(proto::kDitSymlink);
        dit_file->set_path(path.string());
        dit_file->set_canonical(boost::filesystem::canonical(path).string());
        dit_file->set_size(1);
        dit_file->set_perms(boost::filesystem::status(path).permissions());
        if (!boost::algorithm::ends_with(path.string(), "/")) {
            done->Run();
            return;
        }
    }

    // directory
    if (boost::filesystem::is_directory(path)) {
        std::string dir_path = request->path();
        if (!boost::algorithm::ends_with(dir_path, "/")) {
            dir_path += "/";
        }
        // add dir self
        proto::DitFileMeta* dit_file = response->add_files();
        dit_file->set_type(proto::kDitDirectory);
        dit_file->set_path(dir_path);
        dit_file->set_size(4096);
        dit_file->set_perms(boost::filesystem::status(path).permissions());

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
            try {
                boost::filesystem::recursive_directory_iterator end_dir_it(path), it;
                travel_files(end_dir_it, it, response, opt_a);
            } catch (const std::exception& e) {
                response->mutable_ret()->set_status(proto::kError);
                response->mutable_ret()->set_message(e.what());
            }
        } else {
            try {
                boost::filesystem::directory_iterator end_dir_it(path), it;
                travel_files(end_dir_it, it, response, opt_a);
            } catch (const std::exception& e) {
                response->mutable_ret()->set_status(proto::kError);
                response->mutable_ret()->set_message(e.what());
            }
        }
    }

    done->Run();
    return;
}

void DitServerImpl::GetFileBlock(::google::protobuf::RpcController* controller,
                                 const proto::GetFileBlockRequest* request,
                                 proto::GetFileBlockResponse* response,
                                 ::google::protobuf::Closure* done) {
    // add to pool
    boost::function<void (void)> handler = \
        boost::bind(&DitServerImpl::HandleGetFileBlock, this, controller, request, response, done);
    pool_.AddTask(handler);
    return;
}

void DitServerImpl::HandleGetFileBlock(::google::protobuf::RpcController* controller,
                                       const proto::GetFileBlockRequest* request,
                                       proto::GetFileBlockResponse* response,
                                       ::google::protobuf::Closure* done) {
    std::string path = request->block().path();
    int64_t offset = request->block().offset();
    int64_t length = request->block().length();
    VLOG(20)
       << "+++ FileBlock, file: [" << path
       << "], offset: [" << offset
       << "], length: [" << length
       << "]";

    int fd = open(path.c_str(), O_RDONLY);
    if(fd > 0) {
        char* ptr = (char*) mmap(NULL, length, PROT_READ, MAP_SHARED, fd, offset);
        close(fd);
        std::string content = std::string(ptr, length);
        proto::DitFileBlock* block = response->mutable_block();
        block->set_path(path);
        block->set_offset(offset);
        block->set_length(length);
        block->set_content(content);
        response->mutable_ret()->set_status(proto::kOk);
        VLOG(20)
            << "--- FileBlock, file: [" << path
            << "], offset: [" << offset
            << "], length: [" << length
            << "]";
        munmap(ptr, length);
    } else {
        LOG(WARNING)
           << "open file failed, file: " << path
           << ", error: " << strerror(errno);
        response->mutable_ret()->set_status(proto::kError);
        response->mutable_ret()->set_message("open file failed");
    }

    done->Run();
}

}
}
