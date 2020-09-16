// Copyright 2018 dit authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "client/dit_client.h"

#include <ctype.h>
#include <error.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <map>
#include <vector>
#include <utility>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <common/thread_pool.h>
#include <gflags/gflags.h>
#include <snappy.h>

#include "proto/dit.pb.h"

DECLARE_string(server_ip);
DECLARE_string(server_port);
DECLARE_int64(file_block_size);
DECLARE_int32(client_thread_num);
DECLARE_int32(request_retry_times);
DECLARE_bool(a);
DECLARE_bool(c);
DECLARE_bool(f);
DECLARE_bool(r);

namespace baidu {
namespace dit {

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

DitClient::DitClient() : pool_(FLAGS_client_thread_num) {
    pthread_mutex_init(&pmutex_, NULL);
    pthread_cond_init(&pcond_, NULL);
}

DitClient::~DitClient() {
    pthread_mutex_destroy(&pmutex_);
    pthread_cond_destroy(&pcond_);
}

bool DitClient::ParsePath(const std::string& raw_path, DitPath& dit_path) {
    if (!boost::algorithm::starts_with(raw_path, "/")) {
        fprintf(stderr, "-parse path failed, path: %s is not start with /\n", raw_path.c_str());
        return false;
    }

    if (boost::algorithm::starts_with(raw_path, "//")) {
        dit_path.server = "";
        dit_path.path = raw_path;
        boost::algorithm::replace_first(dit_path.path, "/", "");
    } else {
        std::vector<std::string> path_parts;
        boost::split(path_parts, raw_path, boost::is_any_of("/"), boost::token_compress_on);
        if (path_parts.size() < 3) {
            fprintf(stderr, "-parse path failed, path: %s in wrong format\n", raw_path.c_str());
            return false;
        }
        dit_path.server = path_parts[1];
        dit_path.path = raw_path;
        boost::algorithm::replace_first(dit_path.path, "/" + dit_path.server, "");
    }

    return true;
}

void DitClient::Ls(int argc, char* argv[]) {
    // parse path
    std::string raw_path = std::string(argv[0]);
    DitPath dit_path;
    bool ret = ParsePath(raw_path, dit_path);
    if (!ret) {
        fprintf(stderr, "-path parse failed\n");
        return;
    }

    proto::DitServer_Stub* stub;
    if (!rpc_client_.GetStub(dit_path.server, &stub)) {
        fprintf(stderr, "-get server stub failed, server: %s\n", dit_path.server.c_str());
        ret = false;
        return;
    }

    proto::GetFileMetaRequest request;
    proto::GetFileMetaResponse response;
    request.set_path(dit_path.path);

    if (FLAGS_a) {
        request.add_options(proto::kOptionAll);
    }
    if (FLAGS_r) {
        request.add_options(proto::kOptionRecursive);
    }

    bool ok = rpc_client_.SendRequest(stub,
                                      &proto::DitServer_Stub::GetFileMeta,
                                      &request, &response, 5, 1);

    if (!ok) {
        fprintf(stderr, "-ls failed\n");
        return;
    }

    for (int i = 0; i < response.files_size(); i++) {
        const proto::DitFileMeta& file = response.files(i);
        fprintf(stdout, "%o\t%10ld\t%s\t\n", file.perms(), file.size(), file.path().c_str());
    }

    return;
}

void DitClient::Cp(int argc, char* argv[]) {
    // parse path
    std::string src_path = std::string(argv[0]);
    std::string dst_path = std::string(argv[1]);

    DitPath src_dit_path, dst_dit_path;
    if (!ParsePath(src_path, src_dit_path)) {
        fprintf(stderr, "-src path parse failed\n");
        return;
    }

    if (!ParsePath(dst_path, dst_dit_path)) {
        fprintf(stderr, "-dst path parse failed\n");
        return;
    }

    src_path = src_dit_path.path;
    dst_path = dst_dit_path.path;

    // cp from remote to local
    if (src_dit_path.server != "" && dst_dit_path.server == "") {
        CpToLocal(src_dit_path, dst_dit_path);
    }

    // cp from local to remote
    if (src_dit_path.server == "" && dst_dit_path.server != "") {
        CpToRemote(src_dit_path, dst_dit_path);
    }

    return;
}

void DitClient::PutFileBlock(proto::DitServer_Stub* stub,
                             const proto::DitFileMeta& file,
                             int64_t offset,
                             int64_t length,
                             char* fp) {
    proto::PutFileBlockRequest* request = new proto::PutFileBlockRequest;
    proto::PutFileBlockResponse* response = new proto::PutFileBlockResponse;
    proto::DitFileBlock* block = request->mutable_block();
    block->set_offset(offset);
    block->set_length(length);
    block->set_path(file.path());

    std::string content;
    snappy::Compress(fp + offset, length, &content);
    block->set_path(file.path());
    block->set_offset(offset);
    block->set_length(content.length());
    block->set_content(content);

    int retry = 0;
    bool ok = true;

    while (retry < FLAGS_request_retry_times) {
        ok = rpc_client_.SendRequest(stub, &proto::DitServer_Stub::PutFileBlock,
                                           request, response, 30, 1);
        if (ok) {
            break;
        }

        retry++;
    }

    if (!ok || proto::kOk != response->ret().status()) {
        {
            MutexLock lock(&mutex_);
            ++done_;
            pthread_cond_signal(&pcond_);
            failed_files_.insert(file.path());
        }
        return;
    }

    {
        MutexLock lock(&mutex_);
        ++done_;
        pthread_cond_signal(&pcond_);
    }

    return;
}

void DitClient::CpToLocal(DitPath& src_dit_path, DitPath& dst_dit_path) {
    proto::DitServer_Stub* src_stub;
    if (!rpc_client_.GetStub(src_dit_path.server, &src_stub)) {
        fprintf(stderr, "-get src server stub failed, server: %s\n", src_dit_path.server.c_str());
        return;
    }

    // cp from remote to local
    proto::GetFileMetaRequest* request = new proto::GetFileMetaRequest;
    proto::GetFileMetaResponse* response = new proto::GetFileMetaResponse;
    request->add_options(proto::kOptionRecursive);
    request->add_options(proto::kOptionAll);
    request->set_path(src_dit_path.path);
    bool ok = rpc_client_.SendRequest(src_stub,
                                      &proto::DitServer_Stub::GetFileMeta,
                                      request, response, 5, 1);
    if (!ok) {
        fprintf(stderr, "-get %s from %s failed\n", src_dit_path.path.c_str(), src_dit_path.server.c_str());
        if (response->has_ret()) {
            fprintf(stderr, "-error: %s\n", response->ret().message().c_str());
        }
        return;
    }

    fprintf(stdout, "total item %d\n", response->files_size());
    int count = 0;
    int64_t total_size = 0;
    done_ = 0;

    std::vector<std::pair<char*, int64_t> > mmap_ptrs;
    for (int i = 0; i < response->files_size(); i++) {
        const proto::DitFileMeta& file = response->files(i);
        if (proto::kDitDirectory == file.type()) {
            // create_directory
            std::string rel_path = file.path();
            boost::algorithm::replace_first(rel_path, src_dit_path.path, dst_dit_path.path);
            if (!boost::filesystem::exists(rel_path)) {
                boost::filesystem::create_directory(rel_path);
            }
            count += 1;
            {
                MutexLock lock(&mutex_);
                done_++;
            }
        } else {
            // download file
            total_size += file.size();
            std::string file_path = file.path();
            boost::algorithm::replace_first(file_path, src_dit_path.path, dst_dit_path.path);

            int fd = open(file_path.c_str(), O_RDWR | O_CREAT, file.perms());
            if (fd == -1) {
                fprintf(stderr, "-file open failed, file: %s, err: %s\n",
                        file_path.c_str(), strerror(errno));
                failed_files_.insert(file_path);
                continue;
            }

            if (ftruncate(fd, file.size()) < 0) {
                close(fd);
                {
                    MutexLock lock(&mutex_);
                    failed_files_.insert(file_path);
                }
                continue;
            }

            if (0 == file.size()) {
                close(fd);
                continue;
            }

            char* ptr = reinterpret_cast<char*> (mmap(NULL, file.size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
            mmap_ptrs.push_back(std::make_pair(ptr, file.size()));

            if (ptr == MAP_FAILED) {
                fprintf(stderr, "-map failed, file: %s, error: %s!\n", file_path.c_str(), strerror(errno));
                close(fd);
                {
                    MutexLock lock(&mutex_);
                    failed_files_.insert(file_path);
                }
                continue;
            }
            close(fd);

            // split file block
            int64_t block_cout = file.size() / FLAGS_file_block_size;
            int64_t last = file.size() % FLAGS_file_block_size;
            count += last ? block_cout + 1 : block_cout;

            for (int i = 0; i < block_cout; ++i) {
                int64_t offset = FLAGS_file_block_size * i;
                pool_.AddTask(boost::bind(&DitClient::GetFileBlock, this,
                                          src_stub, file,
                                          offset,
                                          FLAGS_file_block_size,
                                          ptr));
            }
            if (last) {
                int64_t offset = file.size() - last;
                pool_.AddTask(boost::bind(&DitClient::GetFileBlock, this,
                                          src_stub, file,
                                          offset, last,
                                          ptr));
            }
        }
    }

    while (done_ < count) {
        pthread_cond_wait(&pcond_, &pmutex_);
    }

    fprintf(stdout, "total size %f mb \n", (total_size * 1.0) / (1024 * 1024));
    std::set<std::string>::iterator it = failed_files_.begin();
    for (; it != failed_files_.end(); ++it) {
        fprintf(stderr, "-cp file failed: %s", (*it).c_str());
    }

    // munmap
    for (unsigned i = 0; i < mmap_ptrs.size(); ++i) {
        munmap(mmap_ptrs[i].first, mmap_ptrs[i].second);
    }
}

void DitClient::CpToRemote(DitPath& src_dit_path, DitPath& dst_dit_path) {
    proto::DitServer_Stub* dst_stub;
    if (!rpc_client_.GetStub(dst_dit_path.server, &dst_stub)) {
        fprintf(stderr, "-get dst server stub failed, server: %s\n", dst_dit_path.server.c_str());
        return;
    }

    proto::PutFileMetaRequest* request = new proto::PutFileMetaRequest;
    proto::PutFileMetaResponse* response = new proto::PutFileMetaResponse;

    boost::filesystem::path path(src_dit_path.path);
    if (!boost::filesystem::exists(path)) {
        fprintf(stdout, "path: %s does not exist", path.string().c_str());
        return;
    }

    // file
    if (boost::filesystem::is_regular_file(path)) {
        proto::DitFileMeta* dit_file = request->add_files();
        dit_file->set_perms(boost::filesystem::status(path).permissions());

        std::string rel_path = path.string();
        boost::algorithm::replace_first(rel_path, src_dit_path.path, dst_dit_path.path);
        fprintf(stdout, "%s", rel_path.c_str());

        dit_file->set_path(path.string());
        dit_file->set_type(proto::kDitFile);
        dit_file->set_size(boost::filesystem::file_size(path));
    }

    // directory
    if (boost::filesystem::is_directory(path)) {
        std::string dir_path = path.string();
        if (!boost::algorithm::ends_with(dir_path, "/")) {
            dir_path += "/";
        }

        // add dir self
        proto::DitFileMeta* dit_file = request->add_files();
        dit_file->set_type(proto::kDitDirectory);
        dit_file->set_path(dir_path);
        dit_file->set_size(4096);
        dit_file->set_perms(boost::filesystem::status(path).permissions());

        try {
            bool opt_a = true;
            boost::filesystem::recursive_directory_iterator end_dir_it(path), it;
            travel_files(end_dir_it, it, request, opt_a);
        } catch (const std::exception& e) {
            fprintf(stdout, "travel path failed: %s", e.what());
        }
    }

    for (int n = 0; n < request->files_size(); n++) {
        std::string rel_path = request->files(n).path();
        boost::algorithm::replace_first(rel_path, src_dit_path.path, dst_dit_path.path);
        request->mutable_files(n)->set_path(rel_path);
    }
    bool ok = rpc_client_.SendRequest(dst_stub,
                                      &proto::DitServer_Stub::PutFileMeta,
                                      request, response, 5, 1);
    if (!ok) {
        fprintf(stderr, "-put %s to %s failed\n", src_dit_path.path.c_str(), dst_dit_path.server.c_str());
        if (response->has_ret()) {
            fprintf(stderr, "-error: %s\n", response->ret().message().c_str());
        }
        return;
    }

    int count = 0;
    int64_t total_size = 0;
    done_ = 0;

    std::vector<std::pair<char*, int64_t> > mmap_ptrs;
    for (int i = 0; i < request->files_size(); i++) {
        const proto::DitFileMeta& file = request->files(i);
        if (proto::kDitDirectory == file.type()) {
            count += 1;
            {
                MutexLock lock(&mutex_);
                done_++;
            }
        } else {
            // upload file
            total_size += file.size();
            std::string file_path = file.path();
            boost::algorithm::replace_first(file_path, dst_dit_path.path, src_dit_path.path);

            int fd = open(file_path.c_str(), O_RDONLY);
            if (fd == -1) {
                fprintf(stderr, "-file open failed, file: %s, err: %s\n",
                        file_path.c_str(), strerror(errno));
                failed_files_.insert(file_path);
                continue;
            }

            char* ptr = reinterpret_cast<char*> (mmap(NULL, file.size(), PROT_READ, MAP_SHARED, fd, 0));
            mmap_ptrs.push_back(std::make_pair(ptr, file.size()));

            if (ptr == MAP_FAILED) {
                fprintf(stderr, "-map failed, file: %s, error: %s!\n", file_path.c_str(), strerror(errno));
                close(fd);
                {
                    MutexLock lock(&mutex_);
                    failed_files_.insert(file_path);
                }
                continue;
            }
            close(fd);

            // split file block
            int64_t block_cout = file.size() / FLAGS_file_block_size;
            int64_t last = file.size() % FLAGS_file_block_size;
            count += last ? block_cout + 1 : block_cout;

            for (int i = 0; i < block_cout; ++i) {
                int64_t offset = FLAGS_file_block_size * i;
                pool_.AddTask(boost::bind(&DitClient::PutFileBlock, this,
                                          dst_stub, file,
                                          offset,
                                          FLAGS_file_block_size,
                                          ptr));
            }
            if (last) {
                int64_t offset = file.size() - last;
                pool_.AddTask(boost::bind(&DitClient::PutFileBlock, this,
                                          dst_stub, file,
                                          offset, last,
                                          ptr));
            }
        }
    }

    while (done_ < count) {
        pthread_cond_wait(&pcond_, &pmutex_);
    }

    fprintf(stdout, "total size %f mb \n", (total_size * 1.0) / (1024 * 1024));
    std::set<std::string>::iterator it = failed_files_.begin();
    for (; it != failed_files_.end(); ++it) {
        fprintf(stderr, "-cp file failed: %s", (*it).c_str());
    }

    // munmap
    for (unsigned i = 0; i < mmap_ptrs.size(); ++i) {
        munmap(mmap_ptrs[i].first, mmap_ptrs[i].second);
    }
}

void DitClient::GetFileBlock(proto::DitServer_Stub* stub,
                             const proto::DitFileMeta& file,
                             int64_t offset,
                             int64_t length,
                             char* fp) {
    proto::GetFileBlockRequest* request = new proto::GetFileBlockRequest;
    proto::GetFileBlockResponse* response = new proto::GetFileBlockResponse;
    proto::DitFileBlock* block = request->mutable_block();
    block->set_offset(offset);
    block->set_length(length);
    block->set_path(file.path());
    int retry = 0;
    bool ok = true;

    while (retry < 3) {
        ok = rpc_client_.SendRequest(stub, &proto::DitServer_Stub::GetFileBlock,
                                     request, response, 30, 1);
        if (ok) {
            break;
        }

        retry++;
    }

    if (!ok || proto::kOk != response->ret().status()) {
        {
            MutexLock lock(&mutex_);
            ++done_;
            pthread_cond_signal(&pcond_);
            failed_files_.insert(file.path());
        }
        return;
    }

    std::string content;
    snappy::Uncompress(response->block().content().c_str(), response->block().length(), &content);
    memcpy(fp + response->block().offset(), content.c_str(), content.size());
    {
        MutexLock lock(&mutex_);
        ++done_;
        pthread_cond_signal(&pcond_);
    }

    return;
}

void DitClient::Rm(int argc, char* argv[]) {
}

}  // namespace dit
}  // namespace baidu
