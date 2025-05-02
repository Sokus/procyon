#ifndef P_FILE_HEADER_GUARD
#define P_FILE_HEADER_GUARD

#include <stdbool.h>
#include <stddef.h>

typedef struct pFileHandle {
#if defined(__PSP__)
    int fd_psp; // SceUID
#elif defined(_WIN32)
    void *handle_win32; // HANDLE
#elif defined(__linux__)
    int fd_linux;
#else
    int fd_dummy;
#endif
} pFileHandle;

bool p_file_open(const char *file_path, pFileHandle *result);
bool p_file_close(pFileHandle file);
bool p_file_write_async(pFileHandle file, void *data, size_t data_size);
bool p_file_poll(pFileHandle file, bool *result);
bool p_file_wait(pFileHandle file);

typedef struct pFileContents {
    void *data;
    size_t size;
} pFileContents;

struct pArena;
pFileContents p_file_read_contents(struct pArena *arena, const char *file_path, bool zero_terminate);

#endif // P_FILE_HEADER_GUARD
