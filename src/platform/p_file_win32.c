#include "platform/p_file.h"
#include "core/p_assert.h"
#include "core/p_arena.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool p_file_open(const char *file_path, pFileHandle *result) {
    bool success = false;
    HANDLE handle_win32 = CreateFileA(
        file_path,
        GENERIC_WRITE | FILE_APPEND_DATA,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (handle_win32 != INVALID_HANDLE_VALUE) {
        success = true;
        *result = (pFileHandle){
            .handle_win32 = handle_win32
        };
    }
    return success;
}

bool p_file_close(pFileHandle file) {
    bool success = CloseHandle(file.handle_win32);
    return success;
}

bool p_file_write_async(pFileHandle file, void *data, size_t data_size) {
    // TODO: Actually do async writing
    bool success = WriteFile(file.handle_win32, data, (DWORD)data_size, NULL, NULL);
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
    HANDLE file_handle = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle == INVALID_HANDLE_VALUE) {
        return result;
    }

    // Get size
    DWORD file_size = 0;
    if ((file_size = GetFileSize(file_handle, &file_size)) == INVALID_FILE_SIZE) {
        CloseHandle(file_handle);
        return result;
    }
    size_t total_size = (size_t)(zero_terminate ? file_size + 1 : file_size);

    void *data = p_arena_alloc(arena, total_size);
    if (data == NULL) {
        CloseHandle(file_handle);
        return result;
    }

    // Read the data
    DWORD bytes_read = 0;
    if (!ReadFile(file_handle, data, file_size, &bytes_read, NULL)) {
        CloseHandle(file_handle);
        return result;
    }
    P_ASSERT(bytes_read == file_size);
    CloseHandle(file_handle);

    result.data = data;
    result.size = total_size;
    if (zero_terminate) {
        ((uint8_t*)data)[file_size] = 0;
    }
    return result;
}
