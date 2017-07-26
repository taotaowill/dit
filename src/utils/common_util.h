#pragma once

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace baidu {
namespace dit {

namespace uuid {
static std::string GenerateUUID() {
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    const std::string tmp = boost::uuids::to_string(uuid);
    return tmp;
}
}

namespace net {
static std::string GetLocalIP() {
    std::string local_ip = "0.0.0.0";
    struct ifaddrs* if_addr = NULL;
    struct ifaddrs* ifa = NULL;
    void* tmp_addr = NULL;

    getifaddrs(&if_addr);
    for (ifa = if_addr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }

        // check it is IP4
        if (ifa->ifa_addr->sa_family == AF_INET) {
            if (0 == strcmp(ifa->ifa_name, "lo")) {
                continue;
            }
            tmp_addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char buf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmp_addr, buf, INET_ADDRSTRLEN);
            local_ip = buf;
        }
    }
    if (if_addr != NULL) freeifaddrs(if_addr);

    return local_ip;
}
}

}
}
