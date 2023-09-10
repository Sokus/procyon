#include "pe_file_io.h"

#include "pe_core.h"

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(PSP)
    #include <pspiofilemgr.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/stat.h>
#endif

#include <stdint.h>


peFileContents pe_file_read_contents(peAllocator allocator, const char *file_path, bool zero_terminate) {
    peFileContents result = {0};
    result.allocator = allocator;

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

    void *data = pe_alloc(allocator, total_size);
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
            pe_free(allocator, data);
            return result;
        }
        PE_ASSERT(bytes_read == file_size);
        CloseHandle(file_handle);
#elif defined(PSP)
        int bytes_read = sceIoRead(file_handle, data, file_size);
        if (bytes_read < 0) {
            sceIoClose(file_handle);
            pe_free(allocator, data);
            return result;
        }
        PE_ASSERT(bytes_read == file_size);
        sceIoClose(file_handle);
#elif defined(__linux__)
        ssize_t bytes_read = read(file_handle, data, file_size);
        if (bytes_read < 0) {
            close(file_handle);
            pe_free(allocator, data);
            return result;
        }
        PE_ASSERT(bytes_read == file_size);
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

void pe_file_free_contents(peFileContents contents) {
    pe_free(contents.allocator, contents.data);
}