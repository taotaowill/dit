// Copyright 2018 dit authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <map>
#include <string>

#include <common/thread_pool.h>
#include <common/mutex.h>
#include <glog/logging.h>
#include <gflags/gflags.h>

#include "proto/dit.pb.h"

namespace baidu {
namespace dit {

class DitServerImpl: public proto::DitServer {
 public:
    DitServerImpl();
    ~DitServerImpl();
    void GetFileMeta(::google::protobuf::RpcController* controller,
                     const proto::GetFileMetaRequest* request,
                     proto::GetFileMetaResponse* response,
                     ::google::protobuf::Closure* done);
    void GetFileBlock(::google::protobuf::RpcController* controller,
                      const proto::GetFileBlockRequest* request,
                      proto::GetFileBlockResponse* response,
                      ::google::protobuf::Closure* done);
    void PutFileMeta(::google::protobuf::RpcController* controller,
                     const proto::PutFileMetaRequest* request,
                     proto::PutFileMetaResponse* response,
                     ::google::protobuf::Closure* done);
    void PutFileBlock(::google::protobuf::RpcController* controller,
                      const proto::PutFileBlockRequest* request,
                      proto::PutFileBlockResponse* response,
                      ::google::protobuf::Closure* done);


 private:
    bool Init();
    void HandleGetFileBlock(::google::protobuf::RpcController* controller,
                            const proto::GetFileBlockRequest* request,
                            proto::GetFileBlockResponse* response,
                            ::google::protobuf::Closure* done);
    void HandlePutFileBlock(::google::protobuf::RpcController* controller,
                            const proto::PutFileBlockRequest* request,
                            proto::PutFileBlockResponse* response,
                            ::google::protobuf::Closure* done);

 private:
    ThreadPool pool_;
    Mutex mutex_;
    std::map<std::string, char*> file_ptrs_;
    std::string endpoint_;
};

}  // namespace dit
}  // namespace baidu
