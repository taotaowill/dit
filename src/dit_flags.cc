#include <gflags/gflags.h>

// service options
DEFINE_string(nexus_addr, "cp01-ocean-893.epc.baidu.com:8868", "root path of dit regsiter on nexus");
DEFINE_string(nexus_root, "/dit", "server list of nexus");
DEFINE_string(nexus_server_prefix, "/server", "the dit server prefix on nexus");
DEFINE_string(server_port, "8999", "the rpc port of dit_server");
DEFINE_int64(file_block_size, 2097152, "file block size limit, bytes");
DEFINE_int32(file_block_timeout, 3000, "get file block timeout, ms");
DEFINE_int32(server_thread_num, 100, "server threads num");
DEFINE_int32(client_thread_num, 64, "client threads num");
DEFINE_int32(client_throughput, 1000, "client throughout rate, MB/s");

// command options
DEFINE_bool(a, false, "do not ignore entries starting with .");
DEFINE_bool(c, false, "control whether color is used to distinguish file");
DEFINE_bool(f, true, "do not prompt before overwriting");
DEFINE_bool(r, false, "travel directories recursively");

