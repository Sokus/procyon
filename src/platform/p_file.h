#ifndef P_FILE_HEADER_GUARD
#define P_FILE_HEADER_GUARD

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define P_MAX_READ_WRITE_SIZE 1 << 30

#if defined(_WIN32)
    typedef void* pFileHandle; // HANDLE
#elif defined(__linux__) || defined(__PSP__) || defined(__3DS__)
    typedef int32_t pFileHandle; // UNIX fd, PSP SceUID
#endif

typedef enum pFileOpenFlag {
#if defined (_WIN32) || defined(__linux__)
    pFO_RDONLY    = 0x00000, // O_RDONLY
    pFO_WRONLY    = 0x00001, // O_WRONLY
    pFO_RDWR      = 0x00002, // O_RDWR
    pFO_CREATE    = 0x00040, // O_CREATE
    pFO_EXCL      = 0x00080, // O_EXCL
    pFO_NOCTTY    = 0x00100, // O_NOCTTY
    pFO_TRUNC     = 0x00200, // O_TRUNC
    pFO_APPEND    = 0x00400, // O_APPEND
    pFO_NONBLOCK  = 0x00800, // O_NONBLOCK
    pFO_SYNC      = 0x01000, // O_SYNC
    pFO_ASYNC     = 0x02000, // O_ASYN
    pFO_DIRECTORY = 0x10000, // O_DIRECTORY
    pFO_CLOEXEC   = 0x80000, // O_CLOEXEC
#elif defined(__PSP__)
    pFO_RDONLY     = 0x0001, // PSP_O_RDONLY
    pFO_WRONLY     = 0x0002, // PSP_O_WRONLY
    pFO_RDWR       = 0x0003, // PSP_O_RDWR
    pFO_NONBLOCK   = 0x0004, // PSP_O_NBLOCK
    pFO_DIRECTORY  = 0x0008, // PSP_O_DIR
    pFO_APPEND     = 0x0100, // PSP_O_APPEND
    pFO_CREATE     = 0x0200, // PSP_O_CREAT
    pFO_TRUNC      = 0x0400, // PSP_O_TRUNC
    pFO_EXCL       = 0x0800, // PSP_O_EXCL
    // pFO_NOWAIT  = 0x8000, // PSP_O_NOWAIT (TODO: what does this flag do?)
    pFO_NOSUPPORT = 0x30000,
    pFO_NOCTTY    = 0x10000,
    pFO_CLOEXEC   = 0x20000,
#elif defined(__3DS__)
    pFO_NULL = 0x0,
#endif
} pFileOpenFlag;

typedef struct pFileInfo {
    char *path;
    char *name;
    bool is_dir;
} pFileInfo;

typedef struct pReadDirResult {
    pFileInfo *file_info;
    int file_count;
} pReadDirResult;

typedef struct pFileContents {
    void *data;
    size_t size;
} pFileContents;

bool p_file_open(const char *path, int mode, int perm, pFileHandle *result);
size_t p_file_write(pFileHandle fd, void *data, size_t data_size);
bool p_file_close(pFileHandle file);

struct pArena;
void p_file_list(const char *dir_path);
pReadDirResult p_file_read_dir(char *dir_path, struct pArena *arena);
bool p_file_directory_exists(const char *dir_path);
pFileContents p_file_read_contents(struct pArena *arena, const char *file_path, bool zero_terminate);

#endif // P_FILE_HEADER_GUARD
