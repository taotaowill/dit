#include "client/dit_client.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>

#include "proto/dit.pb.h"

namespace baidu {
namespace dit {

DitClient::DitClient() {
    Init();
}

bool DitClient::Init() {
    bool ret = true;

    // get servers endpoints
    std::vector<std::string> endpoints;
    endpoints.push_back("127.0.0.1:9999");

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

DitClient::~DitClient() {
    std::map<std::string, proto::DitServer_Stub*>::iterator it = servers_.begin();
    for (; it!=servers_.end(); ++it) {
        if (NULL != it->second) {
            delete it->second;
        }
    }
}

void DitClient::Ls(int argc, char* argv[]) {
    proto::LsRequest request;
    proto::LsResponse response;
    // parse path
    std::string path = std::string(argv[0]);
    request.set_path(path);
    // parse options
    if (argc > 1) {
        std::string options_str = argv[1];
        if (boost::algorithm::starts_with(options_str, "-")) {
            if (options_str.find("a") != std::string::npos) {
                request.add_options(proto::kAll);
            }
        }
    }

    proto::DitServer_Stub* stub = servers_["127.0.0.1:9999"];
    bool ok = rpc_client_.SendRequest(stub,
                                      &proto::DitServer_Stub::Ls,
                                      &request, &response, 5, 1);

    if (!ok) {
        fprintf(stderr, "list failed\n");
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
}

}
}
