// Author: Holodome
// Date: 24.08.2021 
// File: pkby/src/platform/files.h
// Version: 0
//
// Defines os-agnostic API to access files on disk.
//
// @NOTE Filsystem API implements minimal interface for interacting with OS filesystem,
// it does not do streamed IO, like c standard library does (see stream.h).
//
#pragma once
#include "lib/general.h"

enum {
    // 0 in file id could mean not initialized, but this flag explicitly tells that file had errors
    // @NOTE It is still not clear how is better to provide more detailed error details 
    // (probably with use of return codes), but general the fact that file has errors is enough to 
    // stop execution process 
    FILE_FLAG_HAS_ERRORS = 0x1,
    FILE_FLAG_ERROR_NOT_FOUND = 0x2,
    FILE_FLAG_ERROR_ACCESS_DENIED = 0x4,
    FILE_FLAG_IS_CLOSED    = 0x8,
    FILE_FLAG_NOT_OPERATABLE = FILE_FLAG_IS_CLOSED | FILE_FLAG_HAS_ERRORS,
};

// Handle to file object. User has no way of understanding data stored in value
// Because this is not a id but handle, API makes use of static lifetime mechanic builtin in c.
// Everywhere it is used, pointer is passed, because the object itself is strored somewhere
typedef struct {
    // Handle is not guaranteed to have some meaning besides 0 - 
    // that is special value for invalid file handle
    u64 handle;
    u32 flags;
} OS_File_Handle;

#define OS_IS_FILE_VALID(_phandle) ((_phandle)->handle && !((_phandle)->flags & FILE_FLAG_HAS_ERRORS))

enum {
    FILE_MODE_READ,
    FILE_MODE_WRITE,
};

typedef struct {
    u64 storage;
} File_Time;

void os_open_file(OS_File_Handle *handle, const char *filename, u32 mode);
void os_close_file(OS_File_Handle *handle);
u64 os_write_file(OS_File_Handle *file, u64 offset, const void *bf, u64 bf_sz);
u64 os_read_file(OS_File_Handle *file, u64 offset, void *bf, u64 bf_sz);
u64 os_write_stdout(const void *bf, uptr bf_sz);
u64 os_write_stderr(const void *bf, uptr bf_sz);
u64 os_get_file_size(OS_File_Handle *handle);

uptr os_fmt_executable_path(char *bf, uptr bf_sz);
void os_chdir(const char *dir);
void os_fmt_cwd(char *bf, uptr bf_sz);