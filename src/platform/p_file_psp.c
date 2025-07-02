#include "platform/p_file.h"
#include "core/p_assert.h"
#include "core/p_arena.h"

#include <pspkerneltypes.h>
#include <pspiofilemgr.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


bool p_file_open(const char *path, int mode, int perm, pFileHandle *result) {
    SceUID sce_fd = sceIoOpen(path, mode, perm);
    bool success = false;
    if (sce_fd >= 0) {
        success = true;
        *result = (pFileHandle)sce_fd;
    }
    return success;
}

size_t p_file_write(pFileHandle file, void *data, size_t data_size) {
    int write_result = sceIoWrite(file, data, (SceSize)data_size);
    size_t bytes_written = (write_result > 0 ? (size_t)write_result : 0);
    return bytes_written;
}

bool p_file_close(pFileHandle file) {
    int rc_sce = sceIoClose((SceUID)file);
    bool success = (rc_sce >= 0);
    return success;
}

pFileContents p_file_read_contents(pArena *arena, const char *file_path, bool zero_terminate) {
    pFileContents result = {0};

    // Open the file
    SceUID file_handle = sceIoOpen(file_path, PSP_O_RDONLY, 0);
    if (file_handle < 0) {
        return result;
    }

    // Get size
    SceIoStat file_stat;
    if (sceIoGetstat(file_path, &file_stat) < 0) {
        sceIoClose(file_handle);
        return result;
    }
    SceSize file_size = file_stat.st_size;
    size_t total_size = (size_t)(zero_terminate ? file_size + 1 : file_size);

    void *data = p_arena_alloc(arena, total_size);
    if (data == NULL) {
        sceIoClose(file_handle);
        return result;
    }

    // Read the data
    int bytes_read = sceIoRead(file_handle, data, file_size);
    if (bytes_read < 0) {
        sceIoClose(file_handle);
        return result;
    }
    P_ASSERT(bytes_read == file_size);
    sceIoClose(file_handle);

    result.data = data;
    result.size = total_size;
    if (zero_terminate) {
        ((uint8_t*)data)[file_size] = 0;
    }
    return result;
}
