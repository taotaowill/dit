#include <string>

#include <gtest/gtest.h>
#include <gflags/gflags.h>

#include "proto/dit.pb.h"
#include "client/dit_client.h"

TEST(DitClient, ParsePath) {
    std::string raw_path = "/localhost:9999/";
    ::baidu::dit::DitPath dit_path;
    ::baidu::dit::DitClient* c = new ::baidu::dit::DitClient();
    bool ret = c->ParsePath(raw_path, dit_path);
    ASSERT_TRUE(ret);
    EXPECT_EQ(dit_path.server, "localhost:9999");
    EXPECT_EQ(dit_path.path, "/");

    raw_path = "//home//abc/workspace";
    c->ParsePath(raw_path, dit_path);
    EXPECT_EQ(dit_path.server, "");
    EXPECT_EQ(dit_path.path, "/home//abc/workspace");
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
