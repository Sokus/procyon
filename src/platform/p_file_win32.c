#include "platform/p_file.h"
#include "core/p_assert.h"
#include "core/p_arena.h"
#include "core/p_defines.h"
#include "core/p_scratch.h"
#include "core/p_string.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

bool p_file_open(const char *path, int mode, int perm, pFileHandle *result) {
    if (path[0] == '\0') {
        return false; // file doesn't exist
    }

    DWORD access = 0;
    int mode_access_mask = (pFO_RDONLY | pFO_WRONLY | pFO_RDWR);
    switch (mode & mode_access_mask) {
        case pFO_RDONLY: access = FILE_GENERIC_READ; break;
        case pFO_WRONLY: access = FILE_GENERIC_WRITE; break;
        case pFO_RDWR: access = FILE_GENERIC_READ | FILE_GENERIC_WRITE; break;
        default: break;
    }

    if (mode & pFO_CREATE) {
        access |= FILE_GENERIC_WRITE;
    }
    if (mode & pFO_APPEND) {
        access &= ~FILE_GENERIC_WRITE;
    }

    DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    SECURITY_ATTRIBUTES *security_attributes;
    SECURITY_ATTRIBUTES inherit_security_attributes = {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .bInheritHandle = TRUE,
    };
    if ((mode & pFO_CLOEXEC) == 0) {
        security_attributes = &inherit_security_attributes;
    }

    DWORD create_mode = 0;
    int mode_create_mask = (pFO_CREATE | pFO_EXCL | pFO_TRUNC);
    switch (mode & mode_create_mask) {
        case pFO_CREATE: create_mode = OPEN_ALWAYS; break;
        case pFO_TRUNC: create_mode = TRUNCATE_EXISTING; break;
        case pFO_CREATE | pFO_EXCL: create_mode = CREATE_NEW; break;
        case pFO_CREATE | pFO_TRUNC: create_mode = CREATE_ALWAYS; break;
        default: break;
    }

    DWORD flags_and_attributes = (FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS);

    HANDLE handle = CreateFileA(
        path, access, share_mode, security_attributes, create_mode, flags_and_attributes, NULL
    );
    *result = handle;
    return (handle != INVALID_HANDLE_VALUE);
}

bool p_file_close(pFileHandle file) {
    bool success = CloseHandle(file);
    return success;
}

size_t p_file_write(pFileHandle fd, void *data, size_t data_size) {
    size_t total_bytes_written = 0;

    if (data_size == 0) {
        goto end;
    }

    while (total_bytes_written < data_size) {
        size_t bytes_remaining = data_size - total_bytes_written;
        DWORD operation_bytes_to_write = (DWORD)P_MIN(bytes_remaining, P_MAX_READ_WRITE_SIZE);
        DWORD operation_bytes_written;
        bool write_ok = WriteFile(
            (HANDLE)fd,
            (uint8_t*)data+total_bytes_written,
            operation_bytes_to_write,
            &operation_bytes_written,
            NULL
        );
        if (operation_bytes_written <= 0 || !write_ok) {
            goto end;
        }
        total_bytes_written += (size_t)operation_bytes_written;
    }
end:
    return total_bytes_written;
}

void p_file_list(const char *dir_path) {
    char search_path[MAX_PATH];
    unsigned int length;
    for (length = 0; length < MAX_PATH-1-2; length += 1) {
        if (dir_path[length] == '\0') {
            break;
        }
    }
    memcpy(search_path, dir_path, length);
    search_path[length] = '/';
    search_path[length+1] = '*';
    search_path[length+2] = '\0';

    WIN32_FIND_DATAA find_data;
    HANDLE handle = FindFirstFileA(search_path, &find_data);

    if (handle == INVALID_HANDLE_VALUE) {
        printf("Failed to open directory\n");
        return;
    }

    do {
        printf("- %s\n", find_data.cFileName);
    } while (FindNextFileA(handle, &find_data));
}

pReadDirResult p_file_read_dir(char *dir_path, struct pArena *arena) {
    pArenaTemp scratch = p_scratch_begin(&arena, 1);
    pReadDirResult result = {0};
    if (dir_path == NULL) {
        return result;
    }

    pString wildcard_str = P_STRING_LITERAL("/*\0");
    pString dir_path_str = p_string_from_cstring_limit(dir_path, MAX_PATH-wildcard_str.size);
    pString search_path_str = p_string_concatenate(scratch.arena, dir_path_str, wildcard_str);

    WIN32_FIND_DATAA find_data;
    HANDLE find_handle = FindFirstFileA(search_path_str.data, &find_data);
    if (find_handle != INVALID_HANDLE_VALUE) {
        result.file_info = p_arena_alloc(arena, 0);
    } else {
        fprintf(stderr, "Failed to open directory\n");
        return result;
    }

    do {
        pString file_name_str = p_string_from_cstring_limit(find_data.cFileName, MAX_PATH);
        bool is_dot = (0 == p_string_compare(file_name_str, P_STRING_LITERAL(".")));
        bool is_double_dot = (0 == p_string_compare(file_name_str, P_STRING_LITERAL("..")));
        if (is_dot || is_double_dot) continue;

        // p_arena_alloc_empty(arena, sizeof(pFileInfo));
        printf("%.*s\n", P_STRING_EXPAND(file_name_str));
    } while (FindNextFileA(find_handle, &find_data));

    FindClose(find_handle);

    return result;
}

bool p_file_directory_exists(const char *dir_path) {
    DWORD attributes = GetFileAttributesA(dir_path);
    bool is_valid = (attributes != INVALID_FILE_ATTRIBUTES);
    bool is_directory = (attributes && FILE_ATTRIBUTE_DIRECTORY);
    return is_valid && is_directory;
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
