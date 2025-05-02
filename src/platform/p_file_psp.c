#include "platform/p_file.h"
#include "core/p_assert.h"
#include "core/p_arena.h"

#include <pspkerneltypes.h>
#include <pspiofilemgr.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool p_file_open(const char *file_path, pFileHandle *result) {
    int sce_flags = PSP_O_NBLOCK | PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC | PSP_O_APPEND;
    SceUID sce_fd = sceIoOpen(file_path, sce_flags, 0777);
    bool success = false;
    if (sce_fd >= 0) {
        success = true;
        *result = (pFileHandle){
            .fd_psp = sce_fd
        };
        sceIoChangeAsyncPriority(sce_fd, 16);
    }
    return success;
}

bool p_file_close(pFileHandle file) {
    int rc_sce = sceIoClose(file.fd_psp);
    bool success = (rc_sce >= 0);
    return success;
}

bool p_file_write_async(pFileHandle file, void *data, size_t data_size) {
    int sce_rc = sceIoWriteAsync(file.fd_psp, data, (SceSize)data_size);
    bool success = false;
    if (sce_rc >= 0) {
        success = true;
    }
    return success;
}

bool p_file_poll(pFileHandle file, bool *result) {
    SceInt64 sce_result;
    int sce_rc = sceIoPollAsync(file.fd_psp, &sce_result);
    bool success = false;
    switch (sce_rc) {
        case 0: success = true; *result = false; break;
        case 1: success = true; *result = true; break;
        default: break;
    }
    return success;
}

bool p_file_wait(pFileHandle file) {
    SceInt64 sce_result;
    int sce_rc = sceIoWaitAsync(file.fd_psp, &sce_result);
    bool success = false;
    if (sce_rc >= 0) {
        success = true;
    }
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
