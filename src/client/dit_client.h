#pragma once
#include "proto/dit.pb.h"

namespace baidu {
namespace dit {

class DitClient {
public:
    DitClient();
    ~DitClient();
    void Share();
    void List();
    void Download();
};

}
}
