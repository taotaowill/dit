#include "client/dit_client.h"

#include <vector>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <gflags/gflags.h>
#include <common/thread_pool.h>

#include "proto/dit.pb.h"

DECLARE_string(nexus_addr);
DECLARE_string(nexus_root);
DECLARE_string(server_nexus_prefix);


namespace baidu {
namespace dit {

DitClient::DitClient() {
	nexus_ = new InsSDK(FLAGS_nexus_addr);
	ListServers();
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

bool DitClient::ListServers() {
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
            fprintf(stderr, "get server stub failed, server: %s\n", endpoint.c_str());
            ret = false;
            break;
        }
    }

    return ret;
}

bool DitClient::ParsePath(const std::string& raw_path, DitPath& dit_path) {
    if (!boost::algorithm::starts_with(raw_path, "/")) {
        fprintf(stderr, "parse path failed, path: %s is not start with /\n", raw_path.c_str());
        return false;
    }

    std::vector<std::string> path_parts;
    boost::split(path_parts, raw_path, boost::is_any_of("/"), boost::token_compress_on);
    if (path_parts.size() < 3) {
        fprintf(stderr, "parse path failed, path: %s in wrong format\n", raw_path.c_str());
        return false;
    }

    dit_path.server = path_parts[1];
	dit_path.path = raw_path;
    boost::replace_first(dit_path.path, "/" + dit_path.server, "");

    return true;
}

void DitClient::Ls(int argc, char* argv[]) {
    // parse path
    std::string raw_path = std::string(argv[0]);
    DitPath dit_path;
	bool ret = ParsePath(raw_path, dit_path);
    if (!ret) {
        fprintf(stderr, "path parse failed\n");
        return;
    }

    std::map<std::string, proto::DitServer_Stub*>::iterator it = servers_.find(dit_path.server);
    if (it == servers_.end()) {
        fprintf(stderr, "server: %s is not registered\n", dit_path.server.c_str());
        return;
    }
    proto::DitServer_Stub* stub = it->second;
    proto::LsRequest request;
    proto::LsResponse response;
    request.set_path(dit_path.path);
    // parse options
    if (argc > 1) {
        std::string options_str = argv[1];
        if (boost::algorithm::starts_with(options_str, "-")) {
            if (options_str.find("a") != std::string::npos) {
                request.add_options(proto::kAll);
            }
        }
    }
    bool ok = rpc_client_.SendRequest(stub,
                                      &proto::DitServer_Stub::Ls,
                                      &request, &response, 5, 1);

    if (!ok) {
        fprintf(stderr, "list failed\n");
        return;
    }

    for (int i=0; i<response.files_size(); i++) {
        const proto::DitFile& file = response.files(i);
        fprintf(stdout, "%s\n", file.name().c_str());
    }

    return;
}

void DitClient::Cp(int argc, char* argv[]) {
}

void DitClient::Mv(int argc, char* argv[]) {
}

void DitClient::Rm(int argc, char* argv[]) {
}

void DitClient::Put(int argc, char* argv[]) {
}

void DitClient::Get(int argc, char* argv[]) {
    // parse path
    std::string src_path = std::string(argv[0]);
    DitPath dit_path;
    bool ret = ParsePath(src_path, dit_path);
    if (!ret) {
        fprintf(stderr, "path parse failed\n");
        return;
    }

    std::map<std::string, proto::DitServer_Stub*>::iterator it = servers_.find(dit_path.server);
    if (it == servers_.end()) {
        fprintf(stderr, "server: %s is not registered\n", dit_path.server.c_str());
        return;
    }

    proto::DitServer_Stub* stub = it->second;
    proto::GetRequest request;
    request.set_path(dit_path.path);
    proto::GetResponse response;
    bool ok = rpc_client_.SendRequest(stub,
                                      &proto::DitServer_Stub::Get,
                                      &request, &response, 5, 1);
    if (!ok) {
        fprintf(stderr, "get %s failed\n", dit_path.path.c_str());
        return;
    }

    int count = response.files_size();
    fprintf(stdout, "all files: %d", count);

    done_ = 0;
    ThreadPool pool(10);
    for (int i=0; i<response.files_size(); i++) {
        const proto::DitFile& file = response.files(i);
        pool.AddTask(boost::bind(&DitClient::GetFileBlock, this, file));
    }

    while(done_ < count) {
        usleep(500);
    }

    return;
}

void DitClient::GetFileBlock(const proto::DitFile& file) {
    MutexLock lock(&mutex_);
    fprintf(stdout, "%s\t%ld\n", file.name().c_str(), file.size());
    ++done_;
    return;
}

}
}
