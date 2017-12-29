// Copyright 2018 dit authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include <boost/filesystem.hpp>
#include <gflags/gflags.h>
#include <common/timer.h>

#include "dit_client.h"

void print_usage() {
    printf("Usage: dit COMMAND [PATH]... [OPTION]...\n");
    printf("commands:\n");
    printf("  ls PATH [-a] [-c]: list path\n");
    printf("  cp SRC_PATH DST_PATH [-r] [-f]: copy path\n");
    printf("  rm PATH [-r] [-f]: remove path\n");
    printf("options:\n");
    printf("  it is recommend to place options at the end of line\n");
    printf("  -r: travel directories recursively\n");
    printf("  -f: do not prompt before overwriting\n");
    printf("  -a: do not ignore entries starting with .\n");
}

int main(int argc, char* argv[]) {
    ::baidu::common::timer::AutoTimer* timer = NULL;
    // parse command
    if (argc < 2) {
        print_usage();
        return -1;
    }

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
    baidu::dit::DitClient* client = new baidu::dit::DitClient();

    if(!client->Init()) {
        fprintf(stderr, "-init client failed");
        return -1;
    }

    timer = new ::baidu::common::timer::AutoTimer();
    if (strcmp(argv[1], "ls") == 0) {
        if (argc < 3) {
            print_usage();
            return -1;
        }
        client->Ls(argc - 2, argv + 2);
    } else if (strcmp(argv[1], "cp") == 0) {
        if (argc < 4) {
            print_usage();
            return -1;
        }
        client->Cp(argc - 2, argv + 2);
    } else if (strcmp(argv[1], "rm") == 0) {
        if (argc < 3) {
            print_usage();
            return -1;
        }
        client->Rm(argc - 2, argv + 2);
    }
    fprintf(stdout, "total time %f s\n", timer->TimeUsed() * 1.0 / 1000000);
    delete timer;
    timer = NULL;

    return 0;
}

