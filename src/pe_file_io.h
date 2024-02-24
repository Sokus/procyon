#ifndef PE_FILE_IO_H
#define PE_FILE_IO_H

#include <stdbool.h>
#include <stddef.h>

#if defined(__PSP__)
    #include <pspkerneltypes.h>
#endif

typedef struct peFileHandle {
#if defined(__PSP__)
    SceUID fd_psp;
#else
    int fd;
#endif
} peFileHandle;

bool pe_file_open(const char *file_path, peFileHandle *result);
bool pe_file_close(peFileHandle file);
bool pe_file_write(peFileHandle file, void *data, size_t data_size);
bool pe_file_poll(peFileHandle file, bool *result);
bool pe_file_wait(peFileHandle file);

typedef struct peFileContents {
    void *data;
    size_t size;
} peFileContents;

struct peArena;
peFileContents pe_file_read_contents(struct peArena *arena, const char *file_path, bool zero_terminate);

#endif // PE_FILE_IO_H