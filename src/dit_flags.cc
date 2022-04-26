// Copyright 2018 dit authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gflags/gflags.h>

// service options
DEFINE_string(server_ip, "127.0.0.1", "the rpc ip of dit_server");
DEFINE_string(server_port, "8500", "the rpc port of dit_server");
DEFINE_int64(file_block_size, 1048576, "file block size limit, bytes");
DEFINE_int32(request_timeout, 3000, "send file block timeout, ms");
DEFINE_int32(request_retry_times, 10, "rpc max retry times");
DEFINE_int32(server_thread_num, 20, "server threads num");
DEFINE_int32(client_thread_num, 20, "client threads num");

// command options
DEFINE_bool(a, false, "do not ignore entries starting with .");
DEFINE_bool(c, false, "control whether color is used to distinguish file");
DEFINE_bool(f, true, "do not prompt before overwriting");
DEFINE_bool(r, false, "travel directories recursively");
