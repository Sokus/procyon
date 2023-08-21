#include "pe_file_io.h"

#include "pe_core.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

peFileContents pe_file_read_contents(peAllocator allocator, char *file_path, bool zero_terminate) {
    peFileContents result = {0};
#if defined(_WIN32)
    result.allocator = allocator;

    HANDLE file_handle = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle == INVALID_HANDLE_VALUE) {
        return result;
    }

    DWORD file_size;
    if ((file_size = GetFileSize(file_handle, &file_size)) == INVALID_FILE_SIZE) {
        CloseHandle(file_handle);
        return result;
    }

    size_t total_size = zero_terminate ? file_size + 1 : file_size;
    void *data = pe_alloc(allocator, total_size);
    if (data == NULL) {
        CloseHandle(file_handle);
        return result;
    }

    DWORD bytes_read = 0;
    if (!ReadFile(file_handle, data, file_size, &bytes_read, NULL)) {
        CloseHandle(file_handle);
        pe_free(allocator, data);
        return result;
    }
    PE_ASSERT(bytes_read == file_size);
    CloseHandle(file_handle);

    result.data = data;
    result.size = total_size;
    if (zero_terminate) {
        ((uint8_t*)data)[file_size] = 0;
    }
#endif
    return result;
}

void pe_file_free_contents(peFileContents contents);