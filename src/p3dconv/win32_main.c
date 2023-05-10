#include "pe_core.h"

#define M3D_IMPLEMENTATION
#include "m3d.h"

#include <stdio.h>

typedef struct peFileContents {
    peAllocator allocator;
    void *data;
    size_t size;
} peFileContents;

peFileContents pe_file_read_contents(peAllocator allocator, char *file_path, bool zero_terminate) {
    peFileContents result = {0};
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

    return result;
}

static char *pe_m3d_current_path = NULL;
static peAllocator pe_m3d_allocator = {0};

unsigned char *pe_m3d_read_callback(char *filename, unsigned int *size) {
    char file_path[512] = "\0";
    int file_path_length = 0;
    if (pe_m3d_current_path != NULL) {
        int one_past_last_slash_offset = 0;
        for (int i = 0; i < PE_COUNT_OF(file_path)-1; i += 1) {
            if (pe_m3d_current_path[i] == '\0') break;
            if (pe_m3d_current_path[i] == '/') {
                one_past_last_slash_offset = i + 1;
            }
        }
        memcpy(file_path, pe_m3d_current_path, one_past_last_slash_offset);
        file_path_length += one_past_last_slash_offset;
    }
    strcat_s(file_path+file_path_length, sizeof(file_path)-file_path_length-1, filename);

    printf("M3D read callback: %s\n", file_path);
    peFileContents file_contents = pe_file_read_contents(pe_m3d_allocator, file_path, false);
    *size = (unsigned int)file_contents.size;
    return file_contents.data;
}

void pe_m3d_free_callback(void *buffer) {
    pe_free(pe_m3d_allocator, buffer);
}

// *.p3d
// [# of vertices]
//   [positions]
//   [normals]
//   [texcoords]
//   [colors]
// [# of indices]
//   [index]
// [# of meshes]
//   [# of indices]
//   [index offset]
//   [has diffuse color?]
//     [diffuse color]
//   TODO: diffuse color map

// *.pp3d
// [# of meshes]
//   [vertex type]
//   [vertices] (size dependent on vertex type)
//   [indices] (size dependent on vertex type)
//   [has diffuse color?]
//     [diffuse color]
//   TODO: diffuse color map


void pe_p3d_convert(char *source, char *destination) {
    peArena temp_arena;
    pe_arena_init_from_allocator(&temp_arena, pe_heap_allocator(), PE_MEGABYTES(32));
    peAllocator temp_allocator = pe_arena_allocator(&temp_arena);
    pe_m3d_allocator = temp_allocator;
    pe_m3d_current_path = source;

    peFileContents m3d_file_contents = pe_file_read_contents(temp_allocator, source, false);
    m3d_t *m3d = m3d_load(m3d_file_contents.data, pe_m3d_read_callback, pe_m3d_free_callback, NULL);

    m3d_free(m3d);
    pe_arena_free(&temp_arena);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("not enough arguments\n");
        return -1;
    }

    pe_p3d_convert(argv[1], argv[2]);

    return 0;
}