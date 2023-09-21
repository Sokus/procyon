#ifndef PE_FILE_IO_H
#define PE_FILE_IO_H

#include "pe_core.h"

typedef struct peFileContents {
    void *data;
    size_t size;
} peFileContents;

peFileContents pe_file_read_contents(peArena *arena, const char *file_path, bool zero_terminate);

#endif // PE_FILE_IO_H