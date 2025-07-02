#include "platform/p_file.h"
#include "core/p_arena.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#warning p_file unimplemented


bool p_file_open(const char *path, int mode, int perm, pFileHandle *result) {
    return false;
}

size_t p_file_write(pFileHandle file, void *data, size_t data_size) {
    return 0;
}

bool p_file_close(pFileHandle file) {
    return false;
}

pFileContents p_file_read_contents(pArena *arena, const char *file_path, bool zero_terminate) {
    pFileContents result = {0};
    return result;
}
