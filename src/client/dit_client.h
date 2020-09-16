// Copyright 2018 dit authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <pthread.h>
#include <map>
#include <string>
#include <set>

#include <common/mutex.h>

#include "proto/dit.pb.h"
#include "rpc/rpc_client.h"

namespace baidu {
namespace dit {

struct DitPath {
    std::string server;
    std::string path;
    DitPath() {}
};

class DitClient {
 public:
    DitClient();
    ~DitClient();
    void Ls(int argc, char* argv[]);
    void Cp(int argc, char* argv[]);
    void Rm(int argc, char* argv[]);

 private:
    bool ParsePath(const std::string& raw_path, DitPath& dit_path);
    void GetFileBlock(proto::DitServer_Stub* stub,
                      const proto::DitFileMeta& file,
                      int64_t offset,
                      int64_t length,
                      char* fp);
    void PutFileBlock(proto::DitServer_Stub* stub,
                      const proto::DitFileMeta& file,
                      int64_t offset,
                      int64_t length,
                      char* fp);
    void CpToRemote(DitPath& src_dit, DitPath& dst_dit);
    void CpToLocal(DitPath& src_dit, DitPath& dst_dit);
    RpcClient rpc_client_;
    Mutex mutex_;
    int done_;
    std::set<std::string> failed_files_;
    ThreadPool pool_;
    pthread_mutex_t pmutex_;
    pthread_cond_t pcond_;
};

}  // namespace dit
}  // namespace baidu
