#pragma once

#include <string>
#include <ins_sdk.h>
#include <common/mutex.h>

#include "proto/dit.pb.h"
#include "rpc/rpc_client.h"

namespace baidu {
namespace dit {

typedef ::galaxy::ins::sdk::InsSDK InsSDK;
typedef ::galaxy::ins::sdk::ScanResult ScanResult;

struct DitPath {
    std::string server;
    std::string path;

	DitPath() {};
	DitPath(std::string server, std::string path) : server(server), path(path) {};
};

class DitClient {
public:
    DitClient();
    ~DitClient();
    void Ls(int argc, char* argv[]);
    void Rm(int argc, char* argv[]);
    void Cp(int argc, char* argv[]);
    void Mv(int argc, char* argv[]);
    void Get(int argc, char* argv[]);
    void Put(int argc, char* argv[]);
//private:
	bool ListServers();
    bool ParsePath(const std::string& raw_path, DitPath& dit_path);
    void GetFileBlock(const proto::DitFile& file);
    std::map<std::string, proto::DitServer_Stub*> servers_;
    RpcClient rpc_client_;
	InsSDK* nexus_;
    Mutex mutex_;
    int done_;
};

}
}
