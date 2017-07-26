#include <signal.h>
#include <string>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <sofa/pbrpc/pbrpc.h>
#include <common/util.h>

#include "utils/common_util.h"
#include "utils/setting_util.h"
#include "server/dit_server_impl.h"

DECLARE_string(server_port);
static volatile bool s_quit = false;

static void SignalIntHandler(int /*sig*/) {
    s_quit = true;
}

int main(int argc, char* argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    google::SetStderrLogging(google::GLOG_INFO);
    SetupLog("ditd");

    baidu::dit::DitServerImpl* dit_server = new baidu::dit::DitServerImpl();
    std::string endpoint = ::baidu::dit::net::GetLocalIP() + ":" + FLAGS_server_port;
    if (!dit_server->RegisterOnNexus(endpoint)) {
        LOG(FATAL) << "register on nexus failed";
        exit(-1);
    }

    sofa::pbrpc::RpcServerOptions options;
    sofa::pbrpc::RpcServer rpc_server(options);
    if (!rpc_server.RegisterService(static_cast<baidu::dit::proto::DitServer*>(dit_server))) {
        LOG(WARNING) << "failed to register dit server service";
        exit(-2);
    }

    if (!rpc_server.Start(endpoint)) {
        LOG(WARNING) << "failed to start dit server on "<< endpoint;
        exit(-3);
    }

    signal(SIGINT, SignalIntHandler);
    signal(SIGTERM, SignalIntHandler);
    LOG(INFO) << "dit server start ok";

    while (!s_quit) {
        sleep(5);
    }

    return 0;
}
