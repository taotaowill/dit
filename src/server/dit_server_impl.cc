// Copyright 2018 dit authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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
#include <snappy.h>

#include "dit_server_impl.h"
#include "proto/dit.pb.h"
#include "utils/common_util.h"

DECLARE_int32(server_thread_num);

namespace baidu {
namespace dit {

DitServerImpl::DitServerImpl() : pool_(FLAGS_server_thread_num) {}

DitServerImpl::~DitServerImpl() {}

template<class T, class M>
void travel_files(T it, T end_dir_it, M* message, bool opt_a) {
    for (; it != end_dir_it; ++it) {
        try {
            // skip hidden file or dir
            if (!opt_a && it->path().filename().string()[0] == '.') {
                continue;
            }

            if (boost::filesystem::is_directory(*it)) {
                proto::DitFileMeta* dit_file = message->add_files();
                dit_file->set_type(proto::kDitDirectory);
                dit_file->set_path(it->path().string() + "/");
                dit_file->set_size(4096);
                dit_file->set_perms(boost::filesystem::status(it->path()).permissions());
            }

            if (boost::filesystem::is_regular_file(*it)) {
                proto::DitFileMeta* dit_file = message->add_files();
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
        for (int i = 0; i < request->options_size(); i++) {
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
    boost::function<void(void)> handler = boost::bind(
        &DitServerImpl::HandleGetFileBlock,
        this, controller, request, response, done);
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
    if (fd > 0) {
        char* ptr = reinterpret_cast<char*>(mmap(NULL, length, PROT_READ, MAP_SHARED, fd, offset));
        close(fd);
        std::string z_content;
        snappy::Compress(ptr, length, &z_content);
        proto::DitFileBlock* block = response->mutable_block();
        block->set_path(path);
        block->set_offset(offset);
        block->set_length(z_content.length());
        block->set_content(z_content);
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

void DitServerImpl::PutFileMeta(::google::protobuf::RpcController* controller,
                                const proto::PutFileMetaRequest* request,
                                proto::PutFileMetaResponse* response,
                                ::google::protobuf::Closure* done) {
    LOG(INFO) << "put file meta, files: " << request->files_size();
    for (int i = 0; i < request->files_size(); i++) {
        const proto::DitFileMeta& file = request->files(i);
        if (proto::kDitDirectory == file.type()) {
            // create_directory
            std::string file_path = file.path();
            if (!boost::filesystem::exists(file_path)) {
                boost::filesystem::create_directories(file_path);
            }
        } else {
            boost::filesystem::path fpath(file.path());
            if (!boost::filesystem::exists(fpath.parent_path())) {
                boost::filesystem::create_directories(fpath.parent_path());
            }

            std::string file_path = file.path();
            int fd = open(file_path.c_str(), O_RDWR | O_CREAT, file.perms());
            if (fd == -1) {
                fprintf(stderr, "-file open failed, file: %s, err: %s\n",
                        file_path.c_str(), strerror(errno));
                response->mutable_ret()->set_status(proto::kError);
                done->Run();
                return;
            }

            if (ftruncate(fd, file.size()) < 0) {
                close(fd);
                response->mutable_ret()->set_status(proto::kError);
                return;
            }

            if (0 == file.size()) {
                close(fd);
                response->mutable_ret()->set_status(proto::kError);
                return;
            }
        }
    }

    response->mutable_ret()->set_status(proto::kOk);
    done->Run();
}

void DitServerImpl::PutFileBlock(::google::protobuf::RpcController* controller,
                                 const proto::PutFileBlockRequest* request,
                                 proto::PutFileBlockResponse* response,
                                 ::google::protobuf::Closure* done) {
    // add to pool
    boost::function<void(void)> handler = boost::bind(
        &DitServerImpl::HandlePutFileBlock,
        this, controller, request, response, done);
    pool_.AddTask(handler);
    return;
}

void DitServerImpl::HandlePutFileBlock(::google::protobuf::RpcController* controller,
                                       const proto::PutFileBlockRequest* request,
                                       proto::PutFileBlockResponse* response,
                                       ::google::protobuf::Closure* done) {
    std::string path = request->block().path();
    int64_t offset = request->block().offset();
    int64_t length = request->block().length();
    char* ptr = NULL;
    if (file_ptrs_.find(path) != file_ptrs_.end()) {
        ptr = file_ptrs_[path];
    } else {
        int fd = open(path.c_str(), O_RDWR);
        struct stat st;
        if(stat(path.c_str(), &st) != 0) {
            close(fd);
            return;
        }

        if (fd > 0) {
            ptr = reinterpret_cast<char*>(mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
            file_ptrs_[path] = ptr;
            close(fd);
        }
    }

    std::string content;
    snappy::Uncompress(request->block().content().c_str(), length, &content);
    memcpy(ptr + offset, content.c_str(), content.size());
    VLOG(20)
       << "+++ FileBlock, file: [" << path
       << "], offset: [" << offset
       << "], length: [" << content.size()
       << "]";

    response->mutable_ret()->set_status(proto::kOk);
    done->Run();
}

}  // namespace dit
}  // namespace baidu
