#include <stdio.h>

int main(int argc, char *argv[]) {
#if !defined(PE_SERVER_STANDALONE)
    printf("Client\n");
#endif
    printf("Server\n");

    return 0;
}