#include "client/dit_client.h"

#include <ctype.h>
#include <error.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <vector>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/join.hpp>
#include <gflags/gflags.h>
#include <common/thread_pool.h>

#include "proto/dit.pb.h"

DECLARE_string(nexus_addr);
DECLARE_string(nexus_root);
DECLARE_string(server_nexus_prefix);
DECLARE_int64(file_block_size);
DECLARE_int32(client_thread_num);

namespace baidu {
namespace dit {

DitClient::DitClient() : pool_(FLAGS_client_thread_num) {
    nexus_ = new InsSDK(FLAGS_nexus_addr);
    pthread_mutex_init(&pmutex_, NULL);
    pthread_cond_init(&pcond_, NULL);
}

DitClient::~DitClient() {
    delete nexus_;
    std::map<std::string, proto::DitServer_Stub*>::iterator it = servers_.begin();
    for (; it!=servers_.end(); ++it) {
        if (NULL != it->second) {
            delete it->second;
        }
    }
}

bool DitClient::Init() {
    bool ret = true;

    // get servers endpoints
    std::vector<std::string> endpoints;
    std::string full_prefix = FLAGS_nexus_root + FLAGS_server_nexus_prefix;
    ScanResult* result = nexus_->Scan(full_prefix + "/", full_prefix + "/\xff");
    size_t prefix_len = full_prefix.size() + 1;
    while (!result->Done()) {
        const std::string& full_key = result->Key();
        std::string key = full_key.substr(prefix_len);
        endpoints.push_back(key);
        result->Next();
    }

    for (unsigned int i=0; i<endpoints.size(); i++) {
        std::string endpoint = endpoints[i];
        proto::DitServer_Stub* stub;
        if (rpc_client_.GetStub(endpoint, &stub)) {
            servers_[endpoint] = stub;
        } else {
            fprintf(stderr, "-get server stub failed, server: %s\n", endpoint.c_str());
            ret = false;
            break;
        }
    }

    return ret;
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
    // show servers
    if (raw_path == "/") {
        std::map<std::string, proto::DitServer_Stub*>::iterator it = servers_.begin();
        for (; it != servers_.end(); ++it) {
            fprintf(stdout, "/%s\n", it->first.c_str());
        }
        return;
    }

    DitPath dit_path;
    bool ret = ParsePath(raw_path, dit_path);
    if (!ret) {
        fprintf(stderr, "-path parse failed\n");
        return;
    }

    std::map<std::string, proto::DitServer_Stub*>::iterator it = servers_.find(dit_path.server);
    if (it == servers_.end()) {
        fprintf(stderr, "-server: %s is not registered\n", dit_path.server.c_str());
        return;
    }
    proto::DitServer_Stub* stub = it->second;
    proto::GetFileMetaRequest request;
    proto::GetFileMetaResponse response;
    request.set_path(dit_path.path);

    // parse options
    if (argc > 1) {
        std::string options_str = argv[1];
        if (boost::algorithm::starts_with(options_str, "-")) {
            if (options_str.find("a") != std::string::npos) {
                request.add_options(proto::kOptionAll);
            }
            if (options_str.find("r") != std::string::npos) {
                request.add_options(proto::kOptionRecursive);
            }
        }
    }

    bool ok = rpc_client_.SendRequest(stub,
                                      &proto::DitServer_Stub::GetFileMeta,
                                      &request, &response, 5, 1);

    if (!ok) {
        fprintf(stderr, "-ls failed\n");
        return;
    }

    for (int i=0; i<response.files_size(); i++) {
        const proto::DitFileMeta& file = response.files(i);
        fprintf(stdout, "%o\t%13ld\t%s\t\n", file.perms(), file.size(), file.path().c_str());
    }

    return;
}

void DitClient::Cp(int argc, char* argv[]) {
    // parse path
    std::string src_path = std::string(argv[0]);
    std::string dst_path = std::string(argv[1]);

    DitPath src_dit_path, dst_dit_path;
    if(!ParsePath(src_path, src_dit_path)) {
        fprintf(stderr, "-src path parse failed\n");
        return;
    }
    if(!ParsePath(dst_path, dst_dit_path)) {
        fprintf(stderr, "-dst path parse failed\n");
        return;
    }
    src_path = src_dit_path.path;
    dst_path = dst_dit_path.path;

    // restrict
    if (src_dit_path.server == "") {
        fprintf(stderr, "-src server should not be local\n");
        return;
    }

    if (dst_dit_path.server != "") {
        fprintf(stderr, "-dst server must be local\n");
        return;
    }

    std::map<std::string, proto::DitServer_Stub*>::iterator src_it = servers_.find(src_dit_path.server);
    if (src_it == servers_.end()) {
        fprintf(stderr, "-src server: %s is not registered\n", src_dit_path.server.c_str());
        return;
    }

    if (dst_dit_path.server != "") {
        std::map<std::string, proto::DitServer_Stub*>::iterator dst_it = servers_.find(dst_dit_path.server);
        if (dst_it == servers_.end()) {
            fprintf(stderr, "-dst server: %s is not registered\n", dst_dit_path.server.c_str());
            return;
        }
    }

    // cp from remote to local
    proto::DitServer_Stub* src_stub = src_it->second;
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


    fprintf(stdout, "+all files: %d\n", response->files_size());
    int count = 0;
    done_ = 0;
    // is dir
    if (response->files_size() > 1 && dst_path[dst_path.size() - 1] != '/') {
        dst_path += '/';
    }
    for (int i=0; i<response->files_size(); i++) {
        const proto::DitFileMeta& file = response->files(i);
        if (proto::kDitDirectory == file.type()) {
            // create_directory
            std::string rel_path = file.path();
            boost::algorithm::replace_first(rel_path, src_path, dst_path);
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
            std::string file_path = file.path();
            boost::algorithm::replace_first(file_path, src_path, dst_path);

            int fd = open(file_path.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            if(fd == -1) {
                fprintf(stderr, "file open failed, %s\n", strerror(errno));
                continue;
            }

            if (ftruncate(fd, file.size()) < 0) {
                close(fd);
                continue;
            }
            char* ptr = (char*) mmap(NULL, file.size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if(ptr == MAP_FAILED) {
                fprintf(stderr, "map failed!\n");
                close(fd);
                continue;
            }
            close(fd);

            // split file block
            int64_t block_cout = file.size() / FLAGS_file_block_size;
            int64_t last = file.size() % FLAGS_file_block_size;
            count += last ? block_cout + 1 : block_cout;

            for (int i=0; i<block_cout; ++i) {
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

    if (done_ == count) {
        fprintf(stdout, "all file download ok\n");
    }

    //if (munmap(ptr, file.size()) == -1) {
    //    fprintf(stdout, "ummap!\n");
    //}

    return;
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
    bool ret = rpc_client_.SendRequest(stub, &proto::DitServer_Stub::GetFileBlock,
                                       request, response, 30, 1);

    if (!ret) {
        fprintf(stderr, "-get file block rpc failed");
        return;
    }

    if (proto::kOk != response->ret().status()) {
        fprintf(stderr, "-get file block error: %s", response->ret().message().c_str());
        return;
    }

    memcpy(fp + response->block().offset(), response->block().content().c_str(), response->block().length());
    //fprintf(stdout, "+++ FileBlock, file: [%s], offset: [%ld], length: [%ld]\n",
    //        response->block().path().c_str(),
    //        response->block().offset(),
    //        response->block().length());

    {
        MutexLock lock(&mutex_);
        ++done_;
        pthread_cond_signal(&pcond_);
    }

    return;
}

void DitClient::Rm(int argc, char* argv[]) {
}

}
}
