#ifndef P_FILE_HEADER_GUARD
#define P_FILE_HEADER_GUARD

// TODO: Cleanup

#include <stdbool.h>
#include <stddef.h>

#if defined(__PSP__)
    #include <pspkerneltypes.h>
#endif
#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

typedef struct peFileHandle {
#if defined(__PSP__)
    SceUID fd_psp;
#elif defined(_WIN32)
    HANDLE handle_win32;
#elif defined(__linux__)
    int fd_linux;
#else
    int dummy;
#endif
} peFileHandle;

bool pe_file_open(const char *file_path, peFileHandle *result);
bool pe_file_close(peFileHandle file);
bool pe_file_write_async(peFileHandle file, void *data, size_t data_size);
bool pe_file_poll(peFileHandle file, bool *result);
bool pe_file_wait(peFileHandle file);

typedef struct peFileContents {
    void *data;
    size_t size;
} peFileContents;

struct pArena;
peFileContents pe_file_read_contents(struct pArena *arena, const char *file_path, bool zero_terminate);

#endif // P_FILE_HEADER_GUARD

#if defined(P_CORE_IMPLEMENTATION) && !defined(P_FILE_IMPLEMENTATION_GUARD)
#define P_FILE_IMPLEMENTATION_GUARD

#include "p_assert.h"
#include "p_arena.h"

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(PSP)
    #include <pspkerneltypes.h>
    #include <pspiofilemgr.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/stat.h>
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool pe_file_open(const char *file_path, peFileHandle *result) {
    bool success = false;
#if defined(__PSP__)
    int sce_flags = PSP_O_NBLOCK | PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC | PSP_O_APPEND;
    SceUID sce_fd = sceIoOpen(file_path, sce_flags, 0777);
    if (sce_fd >= 0) {
        success = true;
        *result = (peFileHandle){
            .fd_psp = sce_fd
        };
        sceIoChangeAsyncPriority(sce_fd, 16);
    }
#elif defined(_WIN32)
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
        *result = (peFileHandle){
            .handle_win32 = handle_win32
        };
    }
#elif defined(__linux__)
    int flags_linux = O_WRONLY | O_CREAT | O_TRUNC | O_APPEND;
    int fd_linux = open(file_path, flags_linux);
    if (fd_linux >= 0) {
        success = true;
        *result = (peFileHandle){
            .fd_linux = fd_linux
        };
    }
#else
    P_UNIMPLEMENTED();
#endif
    return success;
}

bool pe_file_close(peFileHandle file) {
    bool success = false;
#if defined(__PSP__)
    int rc_sce = sceIoClose(file.fd_psp);
    success = (rc_sce >= 0);
#elif defined(_WIN32)
    success = CloseHandle(file.handle_win32);
#elif defined(__linux__)
    int rc_linux = close(file.fd_linux);
    success = (rc_linux >= 0);
#else
    P_UNIMPLEMENTED();
#endif
    return success;
}

bool pe_file_write_async(peFileHandle file, void *data, size_t data_size) {
    bool success = false;
#if defined(__PSP__)
    int sce_rc = sceIoWriteAsync(file.fd_psp, data, (SceSize)data_size);
    if (sce_rc >= 0) {
        success = true;
    }
#elif defined(_WIN32)
    // TODO: Actually do async writing
    success = WriteFile(file.handle_win32, data, (DWORD)data_size, NULL, NULL);
#elif defined(__linux__)
    // TOOD: Actually do async writing
    ssize_t rc_linux = write(file.fd_linux, data, data_size);
    if (rc_linux >= 0) {
        success = (size_t)rc_linux == data_size;
    }
#else
    P_UNIMPLEMENTED();
#endif
    return success;
}

bool pe_file_poll(peFileHandle file, bool *result) {
    bool success = false;
#if defined(__PSP__)
    SceInt64 sce_result;
    int sce_rc = sceIoPollAsync(file.fd_psp, &sce_result);
    switch (sce_rc) {
        case 0: success = true; *result = false; break;
        case 1: success = true; *result = true; break;
        default: break;
    }
#elif defined(_WIN32) || defined(__linux__)
    // we don't have async writing yet so we can assume
    // the operation is always done
    success = true;
    *result = false;
#else
    P_UNIMPLEMENTED();
#endif
    return success;
}

bool pe_file_wait(peFileHandle file) {
    bool success = false;
#if defined(__PSP__)
    SceInt64 sce_result;
    int sce_rc = sceIoWaitAsync(file.fd_psp, &sce_result);
    if (sce_rc >= 0) {
        success = true;
    }
#elif  defined(_WIN32) || defined(__linux__)
    // we don't have async writing yet so we can assume
    // the operation is always done
    success = true;
#else
    P_UNIMPLEMENTED();
#endif
    return success;
}

peFileContents pe_file_read_contents(pArena *arena, const char *file_path, bool zero_terminate) {
    peFileContents result = {0};

#if defined(_WIN32)
    HANDLE file_handle;
    DWORD file_size;
#elif defined(PSP)
    SceUID file_handle;
    SceSize file_size;
#elif defined(__linux__)
    int file_handle;
    int file_size;
#endif
    size_t total_size;

    // Open the file
    {
#if defined(_WIN32)
        file_handle = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file_handle == INVALID_HANDLE_VALUE) {
            return result;
        }
#elif defined(PSP)
        file_handle = sceIoOpen(file_path, PSP_O_RDONLY, 0);
        if (file_handle < 0) {
            return result;
        }
#elif defined(__linux__)
        file_handle = open(file_path, O_RDONLY);
        if (file_handle < 0) {
            return result;
        }
#endif
    }

    // Get size
    {
#if defined(_WIN32)
        if ((file_size = GetFileSize(file_handle, &file_size)) == INVALID_FILE_SIZE) {
            CloseHandle(file_handle);
            return result;
        }
#elif defined(PSP)
        SceIoStat file_stat;
        if (sceIoGetstat(file_path, &file_stat) < 0) {
            sceIoClose(file_handle);
            return result;
        }
        file_size = file_stat.st_size;
#elif defined(__linux__)
        struct stat file_stat;
        if (fstat(file_handle, &file_stat) < 0) {
            close(file_handle);
            return result;
        }
        file_size = file_stat.st_size;
#endif
        total_size = (size_t)(zero_terminate ? file_size + 1 : file_size);
    }

    void *data = p_arena_alloc(arena, total_size);
    if (data == NULL) {
#if defined(_WIN32)
        CloseHandle(file_handle);
#elif defined(PSP)
        sceIoClose(file_handle);
#elif defined(__linux__)
        close(file_handle);
#endif
        return result;
    }

    // Read the data
    {
#if defined(_WIN32)
        DWORD bytes_read;
        if (!ReadFile(file_handle, data, file_size, &bytes_read, NULL)) {
            CloseHandle(file_handle);
            return result;
        }
        P_ASSERT(bytes_read == file_size);
        CloseHandle(file_handle);
#elif defined(PSP)
        int bytes_read = sceIoRead(file_handle, data, file_size);
        if (bytes_read < 0) {
            sceIoClose(file_handle);
            return result;
        }
        P_ASSERT(bytes_read == file_size);
        sceIoClose(file_handle);
#elif defined(__linux__)
        ssize_t bytes_read = read(file_handle, data, file_size);
        if (bytes_read < 0) {
            close(file_handle);
            return result;
        }
        P_ASSERT(bytes_read == file_size);
        close(file_handle);
#endif
    }

    result.data = data;
    result.size = total_size;
    if (zero_terminate) {
        ((uint8_t*)data)[file_size] = 0;
    }
    return result;
}

#endif // P_CORE_IMPLEMENTATION
