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
};

class DitClient {
public:
    DitClient();
    ~DitClient();
    void Ls(int argc, char* argv[]);
    void Cp(int argc, char* argv[]);
    void Rm(int argc, char* argv[]);
//private:
	bool Init();
    bool ParsePath(const std::string& raw_path, DitPath& dit_path);
    void GetFileBlock(proto::DitServer_Stub* stub,
                      const proto::DitFileMeta& file,
                      int64_t offset,
                      int64_t length,
                      const std::string& src_path,
                      const std::string& dst_path);
    std::map<std::string, proto::DitServer_Stub*> servers_;
    RpcClient rpc_client_;
	InsSDK* nexus_;
    Mutex mutex_;
    int done_;
    ThreadPool pool_;
};

}
}
