#include <stdio.h>

#include "core/p_assert.h"
#include "platform/p_file.h"
#include "core/p_scratch.h"

int main(int argc, char *argv[]) {
    // p_file_list("./res");
    // pFileHandle out_file;
    // if (!p_file_open2("./test.txt", pFO_WRONLY|pFO_CREATE|pFO_TRUNC, 0, &out_file)) {
    //     printf("couldn't open file\n");
    //     return 1;
    // }

    // char msg[] = "This is a test message\n"
    //     "Nothing out of the ordinary...\n";
    // bool write_ok = p_file_write2(out_file, msg, sizeof(msg)-1, NULL);
    // P_ASSERT(write_ok);

    // p_file_close(out_file);

    pArenaTemp scratch = p_scratch_begin(NULL, 0);
    p_file_read_dir("./res", scratch.arena);
    p_scratch_end(scratch);

    return 0;
}
