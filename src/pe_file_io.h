#ifndef PE_FILE_IO_H
#define PE_FILE_IO_H

#include "pe_core.h"

typedef struct peFileContents {
    peAllocator allocator;
    void *data;
    size_t size;
} peFileContents;

peFileContents pe_file_read_contents(peAllocator allocator, char *file_path, bool zero_terminate);
void pe_file_free_contents(peFileContents contents);

#endif // PE_FILE_IO_H