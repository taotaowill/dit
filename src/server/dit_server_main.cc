// Copyright 2018 dit authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <signal.h>
#include <string>

#include <boost/filesystem.hpp>
#include <common/util.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <sofa/pbrpc/pbrpc.h>

#include "server/dit_server_impl.h"
#include "utils/common_util.h"
#include "utils/setting_util.h"

DECLARE_string(server_port);
DECLARE_string(server_ip);
static volatile bool s_quit = false;

static void SignalIntHandler(int /*sig*/) {
    s_quit = true;
}

int main(int argc, char* argv[]) {
    bool has_flagfile = false;
    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], "--flagfile", 10) == 0) {
            has_flagfile = true;
            break;
        }
    }
    if (!has_flagfile) {
        char c_exe_path[PATH_MAX];
        readlink("/proc/self/exe", c_exe_path, PATH_MAX);
        boost::filesystem::path exe_path(c_exe_path);
        std::string s_flag_path = exe_path.parent_path().string() + "/dit.flag";
        std::string flagfile = "--flagfile=" + s_flag_path;
        if (boost::filesystem::exists(s_flag_path)) {
            argv[argc] = strdup(flagfile.c_str());
            argc++;
        }
    }
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    google::SetStderrLogging(google::GLOG_INFO);
    SetupLog("ditd");

    baidu::dit::DitServerImpl* dit_server = new baidu::dit::DitServerImpl();
    std::string endpoint;
    if (FLAGS_server_ip != "") {
      endpoint = FLAGS_server_ip + ":" + FLAGS_server_port;
    } else {
      endpoint = ::baidu::dit::net::GetLocalIP() + ":" + FLAGS_server_port;
    }

    sofa::pbrpc::RpcServerOptions options;
    sofa::pbrpc::RpcServer rpc_server(options);
    if (!rpc_server.RegisterService(static_cast<baidu::dit::proto::DitServer*>(dit_server))) {
        LOG(WARNING) << "failed to register dit server service";
        exit(-2);
    }

    std::string inner_endpoint = "0.0.0.0:" + FLAGS_server_port;
    if (!rpc_server.Start(inner_endpoint)) {
        LOG(WARNING) << "failed to start dit server on "<< endpoint;
        exit(-3);
    }

    signal(SIGINT, SignalIntHandler);
    signal(SIGTERM, SignalIntHandler);
    LOG(INFO) << "dit server start ok, bind " << inner_endpoint;

    while (!s_quit) {
        sleep(5);
    }

    return 0;
}
