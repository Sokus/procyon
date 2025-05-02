#include "platform/p_file.h"
#include "core/p_assert.h"
#include "core/p_arena.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool p_file_open(const char *file_path, pFileHandle *result) {
    int flags_linux = O_WRONLY | O_CREAT | O_TRUNC | O_APPEND;
    int fd_linux = open(file_path, flags_linux);
    bool success = false;
    if (fd_linux >= 0) {
        success = true;
        *result = (pFileHandle){
            .fd_linux = fd_linux
        };
    }
    return success;
}

bool p_file_close(pFileHandle file) {
    int rc_linux = close(file.fd_linux);
    bool success = (rc_linux >= 0);
    return success;
}

bool p_file_write_async(pFileHandle file, void *data, size_t data_size) {
    // TOOD: Actually do async writing
    ssize_t rc_linux = write(file.fd_linux, data, data_size);
    bool success = false;
    if (rc_linux >= 0) {
        success = (size_t)rc_linux == data_size;
    }
    return success;
}

bool p_file_poll(pFileHandle file, bool *result) {
    // we don't have async writing yet so we can assume
    // the operation is always done
    bool success = true;
    *result = false;
    return success;
}

bool p_file_wait(pFileHandle file) {
    // we don't have async writing yet so we can assume
    // the operation is always done
    bool success = true;
    return success;
}

pFileContents p_file_read_contents(pArena *arena, const char *file_path, bool zero_terminate) {
    pFileContents result = {0};

    // Open the file
    int file_handle = open(file_path, O_RDONLY);
    if (file_handle < 0) {
        return result;
    }

    // Get size
    struct stat file_stat;
    if (fstat(file_handle, &file_stat) < 0) {
        close(file_handle);
        return result;
    }
    int file_size = file_stat.st_size;
    size_t total_size = (size_t)(zero_terminate ? file_size + 1 : file_size);

    void *data = p_arena_alloc(arena, total_size);
    if (data == NULL) {
        close(file_handle);
        return result;
    }

    // Read the data
    ssize_t bytes_read = read(file_handle, data, file_size);
    if (bytes_read < 0) {
        close(file_handle);
        return result;
    }
    P_ASSERT(bytes_read == file_size);
    close(file_handle);

    result.data = data;
    result.size = total_size;
    if (zero_terminate) {
        ((uint8_t*)data)[file_size] = 0;
    }
    return result;
}
