#pragma once

#include <string>

#include "proto/dit.pb.h"
#include "rpc/rpc_client.h"

namespace baidu {
namespace dit {

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
private:
    bool Init();
    std::map<std::string, proto::DitServer_Stub*> servers_;
    RpcClient rpc_client_;
};

}
}
