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
#include "general.h"

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
} OSFileHandle;

#define OS_IS_FILE_VALID(_phandle) ((_phandle)->handle && !((_phandle)->flags & FILE_FLAG_HAS_ERRORS))

enum {
    FILE_MODE_READ,
    FILE_MODE_WRITE,
};

typedef struct {
    u64 storage;
} File_Time;

#define OS_OPEN_FILE(_name) void _name(OSFileHandle *handle, const char *filename, u32 mode)
typedef OS_OPEN_FILE(OSOpenFile);
#define OS_CLOSE_FILE(_name) void _name(OSFileHandle *handle)
typedef OS_CLOSE_FILE(OSCLoseFile);
#define OS_WRITE_FILE(_name) u64 _name(OSFileHandle *file, u64 offset, const void *bf, u64 bf_sz)
typedef OS_WRITE_FILE(OSWriteFile);
#define OS_READ_FILE(_name) u64 _name(OSFileHandle *file, u64 offset, void *bf, u64 bf_sz)
typedef OS_READ_FILE(OSReadFile);
#define OS_WRITE_STDOUT(_name) u64 _name(const void *bf, uptr bf_sz)
typedef OS_WRITE_STDOUT(OSWriteStdout);
#define OS_WRITE_STDERR(_name) u64 _name(const void *bf, uptr bf_sz)
typedef OS_WRITE_STDERR(OSWriteStderr);
#define OS_GET_FILE_SIZE(_name) u64 _name(OSFileHandle *handle)
typedef OS_GET_FILE_SIZE(OSGetFileSize);

typedef struct {
    OSOpenFile *open_file;
    OSCLoseFile *close_file;
    OSWriteFile *write_file;
    OSReadFile *read_file;
    OSGetFileSize *get_file_size;
    OSWriteStdout *write_stdout;
    OSWriteStderr *write_stderr;
} OS_Api;