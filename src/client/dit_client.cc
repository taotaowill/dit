#include "client/dit_client.h"

#include <stdio.h>
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

namespace baidu {
namespace dit {

DitClient::DitClient() : pool_(10) {
	nexus_ = new InsSDK(FLAGS_nexus_addr);
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
        fprintf(stdout, "+%s\n", file.path().c_str());
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
        fprintf(stderr, "-dst server must not be local\n");
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

    int count = response->files_size();
    fprintf(stdout, "+all files: %d\n", count);
    done_ = 0;
    for (int i=0; i<response->files_size(); i++) {
        const proto::DitFileMeta& file = response->files(i);
        if (proto::kDitDirectory == file.type()) {
            // create_directory
            std::string rel_path = file.path();
            boost::algorithm::replace_first(rel_path, src_path, dst_path);
            fprintf(stdout, "+[%s] [%s]\n", file.path().c_str(), rel_path.c_str());
            if (!boost::filesystem::exists(rel_path)) {
                boost::filesystem::create_directory(rel_path);
            }
            {
                MutexLock lock(&mutex_);
                done_++;
            }
        } else {
            // download file
            pool_.AddTask(boost::bind(&DitClient::GetFileBlock, this, src_stub, file,
                                     src_path, dst_path));
        }
    }

    while(done_ < count) {
        usleep(500);
    }

    return;
}

void DitClient::GetFileBlock(proto::DitServer_Stub* stub,
                             const proto::DitFileMeta& file,
                             const std::string& src_path,
                             const std::string& dst_path) {
    proto::GetFileBlockRequest* request = new proto::GetFileBlockRequest;
    proto::GetFileBlockResponse* response = new proto::GetFileBlockResponse;
    proto::DitFileBlock* block = request->mutable_block();
    block->set_offset(0);
    block->set_length(file.size());
    block->set_path(file.path());
    boost::function<void (const proto::GetFileBlockRequest*, proto::GetFileBlockResponse*, bool, int)> callback;
    callback = boost::bind(&DitClient::GetFileBlockCallback, this, src_path, dst_path, _1, _2, _3, _4);
    rpc_client_.AsyncRequest(stub, &proto::DitServer_Stub::GetFileBlock,
                             request, response,
                             callback, 30, 1);
    return;
}

void DitClient::GetFileBlockCallback(const std::string& src_path,
                                     const std::string& dst_path,
                                     const proto::GetFileBlockRequest* request,
                                     proto::GetFileBlockResponse* response,
                                     bool failed, int /*error*/) {
    fprintf(stdout, "--- FileBlock, file: [%s], offset: [%ld], length: [%ld]\n",
            response->block().path().c_str(),
            response->block().offset(),
            response->block().length());

    std::string path = response->block().path();
    boost::algorithm::replace_first(path, src_path, dst_path);
    FILE* fp;
    if ((fp = fopen(path.c_str(), "wb")) != NULL) {
        const proto::DitFileBlock& block = response->block();
        fseek(fp, 0, SEEK_SET);
        fwrite(block.content().c_str(), 1, block.length(), fp);
        fclose(fp);
        fprintf(stdout, "+++ FileBlock, file: [%s], offset: [%ld], length: [%ld]\n",
                block.path().c_str(),
                block.offset(),
                block.length());
    } else {
        fprintf(stderr, "-write file block failed, file: %s\n", request->block().path().c_str());
    }
    { 
        MutexLock lock(&mutex_);
        ++done_;
    }
    return;
}

void DitClient::Rm(int argc, char* argv[]) {
}

}
}
