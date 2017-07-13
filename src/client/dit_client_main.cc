#include <string>

#include "dit_client.h"

void print_usage() {
    printf("Usage:\ndit <command> path\n");
    printf("\tcommand:\n");
    printf("\t\tls <path> [-options]: list the directory, options: a|c \n");
    printf("\t\tcp <srcpath> <destpath> [-options]: copy directory or file, options: r|f\n");
    printf("\t\tmv <srcpath> <destpath> [-options]: rename directory or file, options: r|f\n");
    printf("\t\trm <path> [-options]: remove direcory or file, options: r|f\n");
    printf("\t\tget <ditfile> <localfile> : copy file from dit to local\n");
    printf("\t\tput <localfile> <ditfile> : copy file from local to dit\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return -1;
    }

    baidu::dit::DitClient* client = new baidu::dit::DitClient();
    if(!client->Init()) {
        fprintf(stderr, "-init client failed");
        return -1;
    }

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
    } else if (strcmp(argv[1], "rm") == 0) {
        if (argc < 3) {
            print_usage();
            return -1;
        }
        client->Rm(argc - 2, argv + 2);
    }

    return 0;
}

