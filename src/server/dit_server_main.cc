#include <signal.h>
#include <string>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <sofa/pbrpc/pbrpc.h>

#include "utils/setting_util.h"
#include "server/dit_server_impl.h"

DECLARE_string(dit_server_port);
static volatile bool s_quit = false;

static void SignalIntHandler(int /*sig*/) {
    s_quit = true;
}

int main(int argc, char* argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    SetupLog("ditd");

    baidu::dit::DitServerImpl* dit_server = new baidu::dit::DitServerImpl();
    sofa::pbrpc::RpcServerOptions options;
    sofa::pbrpc::RpcServer rpc_server(options);
    if (!rpc_server.RegisterService(static_cast<baidu::dit::proto::DitServer*>(dit_server))) {
        LOG(WARNING) << "failed to register dit server service";
        exit(-1);
    }
    std::string endpoint = "0.0.0.0:" + FLAGS_dit_server_port;
    if (!rpc_server.Start(endpoint)) {
        LOG(WARNING) << "failed to start dit server on "<< endpoint;
        exit(-2);
    }

    signal(SIGINT, SignalIntHandler);
    signal(SIGTERM, SignalIntHandler);
    LOG(INFO) << "dit server start ok";

    while (!s_quit) {
        sleep(1);
    }

    return 0;
}
