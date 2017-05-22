#include "dit_client.h"

int main(int argc, char* argv[]) {
    baidu::dit::DitClient* client = new baidu::dit::DitClient();
    client->List();

    return 0;
}
