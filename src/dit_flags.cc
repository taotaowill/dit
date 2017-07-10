#include <gflags/gflags.h>

DEFINE_string(nexus_addr, "/dit", "root path of dit regsiter on nexus");
DEFINE_string(nexus_root, "127.0.0.1:8868", "server list of nexus");
DEFINE_string(server_port, "9999", "the rpc port of dit_server");
DEFINE_string(server_nexus_prefix, "/server", "the dit server prefix on nexus");

