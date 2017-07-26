#include <stdio.h>
#include <string>
#include <gtest/gtest.h>
#include "utils/common_util.h"

TEST(uuid, GenerateUUID) {
    std::string uuid = baidu::dit::uuid::GenerateUUID();
}

TEST(net, GetLocalIP) {
    std::string local_ip = baidu::dit::net::GetLocalIP();
    fprintf(stdout, "local ip: %s\n", local_ip.c_str());
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
