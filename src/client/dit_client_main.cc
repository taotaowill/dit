#include <string>

#include "dit_client.h"

void print_usage() {
    printf("Use:\ndit <command> path\n");
    printf("\t command:\n");
    printf("\t    ls <path> [-options]: list the directory, options: a|c \n");
    printf("\t    cp <srcpath> <destpath> [-options]: copy directory or file, options: r|f\n");
    printf("\t    mv <srcpath> <destpath> [-options]: rename directory or file, options: r|f\n");
    printf("\t    rm <path> [-options]: remove direcory or file, options: r|f\n");
    printf("\t    get <ditfile> <localfile> : copy file from dit to local\n");
    printf("\t    put <localfile> <ditfile> : copy file from local to dit\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return -1;
    }

    baidu::dit::DitClient* client = new baidu::dit::DitClient();
    if (strcmp(argv[1], "ls") == 0) {
        if (argc < 3) {
            print_usage();
            return -1;
        }
        client->Ls(argc - 2, argv + 2);
    } else if (strcmp(argv[1], "cp") == 0) {
        if (argc < 4) {
            print_usage();
            return -1;
        }
        client->Cp(argc - 2, argv + 2);
    } else if (strcmp(argv[1], "mv") == 0) {
        if (argc < 4) {
            print_usage();
            return -1;
        }
        client->Mv(argc - 2, argv + 2);
    } else if (strcmp(argv[1], "rm") == 0) {
        if (argc < 3) {
            print_usage();
            return -1;
        }
        client->Rm(argc - 2, argv + 2);
    } else if (strcmp(argv[1], "put") == 0) {
        if (argc < 4) {
            print_usage();
            return -1;
        }
        client->Put(argc - 2, argv + 2);
    } else if (strcmp(argv[1], "get") == 0 ) {
        if (argc < 4) {
            print_usage();
            return -1;
        }
        client->Get(argc - 2, argv + 2);
    }

    return 0;
}
