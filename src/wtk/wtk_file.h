#ifndef WTK_FILE_H
#define WTK_FILE_H

#include <stdio.h>

#include "wtk_common.h"
#include "wtk_str.h"

#if wtk_win_os
#include "wtk_windows.h"
#endif // wtk_win_os

typedef struct {
#if wtk_win_os
    HANDLE hnd;
#elif wtk_lin_os // wtk_win_os
    int fd;
#endif // wtk_lin_os

    char* file_path;
} WtkFile;

typedef enum {
    WTK_FILE_MODE_OPEN,
    WTK_FILE_MODE_CREATE,
} WtkFileMode;

typedef enum {
    WTK_FILE_ACCESS_READ,
    WTK_FILE_ACCESS_WRITE,
    WTK_FILE_ACCESS_READWRITE,
} WtkFileAccess;

typedef enum {
    WTK_FILE_SHARE_NONE,
    WTK_FILE_SHARE_READ,
    WTK_FILE_SHARE_WRITE,
    WTK_FILE_SHARE_READWRITE,
} WtkFileShare;

// file

WtkFile* wtk_file_open(const char* path, WtkFileMode mode, WtkFileAccess access, WtkFileShare share);
void wtk_file_close(WtkFile* file);

int64_t wtk_file_length(const WtkFile* file);

int64_t wtk_file_read_at(WtkFile* file, uint8_t* out_data, int64_t offset, int64_t length);

// path

char* wtk_file_path_filename(const char* path);

#endif // WTK_FILE_H
