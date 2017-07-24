#include <gflags/gflags.h>

DEFINE_string(nexus_addr, "cp01-ocean-893.epc.baidu.com:8868", "root path of dit regsiter on nexus");
DEFINE_string(nexus_root, "/dit", "server list of nexus");
DEFINE_string(server_port, "8999", "the rpc port of dit_server");
DEFINE_string(server_nexus_prefix, "/server", "the dit server prefix on nexus");
DEFINE_int64(file_block_size, 1048576, "file block size limit, bytes");
DEFINE_int64(dir_size, 4096, "directory entriy size");
DEFINE_int32(get_file_block_timeout, 3000, "get file block timeout, seconds");
DEFINE_int32(server_thread_num, 10, "server threads num");

