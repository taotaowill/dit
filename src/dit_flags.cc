#include <gflags/gflags.h>

DEFINE_string(nexus_addr, "cp01-ocean-893.epc.baidu.com:8868", "root path of dit regsiter on nexus");
DEFINE_string(nexus_root, "/dit", "server list of nexus");
DEFINE_string(server_port, "9999", "the rpc port of dit_server");
DEFINE_string(server_nexus_prefix, "/server", "the dit server prefix on nexus");
DEFINE_int64(file_blcok_size, 1024, "file block size, bytes");
DEFINE_int32(get_file_block_timeout, 3000, "get file block timeout, seconds");

