#include "wtk_file.h"

#if wtk_lin_os
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#endif

// file

WtkFile* wtk_file_open(const char* path, WtkFileMode mode, WtkFileAccess access, WtkFileShare share)
{
#if wtk_win_os
    DWORD creation = 0;
    switch (mode) {
        case WTK_FILE_MODE_OPEN:
            creation = OPEN_EXISTING;
            break;
        case WTK_FILE_MODE_CREATE:
            creation = OPEN_ALWAYS;
            break;
    }

    DWORD desired_access = 0;
    switch (access) {
        case WTK_FILE_ACCESS_READ:
            desired_access = GENERIC_READ;
            break;
        case WTK_FILE_ACCESS_WRITE:
            desired_access = GENERIC_WRITE;
            break;
        case WTK_FILE_ACCESS_READWRITE:
            desired_access = GENERIC_READ | GENERIC_WRITE;
            break;
    }

    DWORD share_mode = 0;
    switch (share) {
        case WTK_FILE_SHARE_NONE:
            share_mode = 0;
            break;
        case WTK_FILE_SHARE_READ:
            share_mode = FILE_SHARE_READ;
            break;
        case WTK_FILE_SHARE_WRITE:
            share_mode = FILE_SHARE_WRITE;
            break;
        case WTK_FILE_SHARE_READWRITE:
            share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
            break;
    }

    LPWSTR path_wstr = wtk_utf8_to_lpwstr(path);
    if (path_wstr == NULL)
        return NULL;

    HANDLE hnd = CreateFileW(path_wstr, desired_access, share_mode, NULL, creation,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);

    if (hnd == INVALID_HANDLE_VALUE) {
        free(path_wstr);
        return NULL;
    }

    // resolve to a full path, like realpath() on linux
    // todo: support \\?\ paths
    char* full_path = NULL;
    DWORD len = GetFullPathNameW(path_wstr, 0, NULL, NULL);
    if (len > 0) {
        LPWSTR full_wstr = wtk_malloc(LPWSTR, (size_t)len * sizeof(wchar_t));
        if (full_wstr != NULL) {
            DWORD written = GetFullPathNameW(path_wstr, len, full_wstr, NULL);
            if (written > 0 && written < len)
                full_path = wtk_lpcwstr_to_utf8(full_wstr);

            free(full_wstr);
        }
    }

    free(path_wstr);

    WtkFile* file = wtk_malloc(WtkFile*, sizeof(WtkFile));
    file->hnd = hnd;
    file->file_path = full_path;
    return file;
#elif wtk_lin_os
    int flags = O_CLOEXEC;

    switch (mode) {
        case WTK_FILE_MODE_OPEN:
            // none
            break;
        case WTK_FILE_MODE_CREATE:
            flags |= O_CREAT;
            break;
    }

    switch (access) {
        case WTK_FILE_ACCESS_READ:
            flags |= O_RDONLY;
            break;
        case WTK_FILE_ACCESS_WRITE:
            flags |= O_WRONLY;
            break;
        case WTK_FILE_ACCESS_READWRITE:
            flags |= O_RDWR;
            break;
    }

    // linux doesn't support the granuality that windows has, so
    // we'll copy what .net does: none = exclusive, others = shared
    int flock_flags = 0;
    switch (share) {
        case WTK_FILE_SHARE_NONE:
            flock_flags = LOCK_EX;
            break;
        case WTK_FILE_SHARE_READ:
        case WTK_FILE_SHARE_WRITE:
        case WTK_FILE_SHARE_READWRITE:
            flock_flags = LOCK_SH;
            break;
    }

    int fd = open(path, flags);
    if (fd == -1) {
        return NULL;
    }

    if (flock(fd, flock_flags) == -1) {
        close(fd);
        return NULL;
    }

    WtkFile* file = wtk_malloc(WtkFile*, sizeof(WtkFile));
    file->fd = fd;
    file->file_path = realpath(path, NULL);
    return file;
#endif // wtk_lin_os

    return NULL;
}

void wtk_file_close(WtkFile* file)
{
#if wtk_win_os
    CloseHandle(file->hnd);

    if (file->file_path != NULL)
        free(file->file_path);

    free(file);
#elif wtk_lin_os
    close(file->fd);

    if (file->file_path != NULL)
        free(file->file_path);

    free(file);
#endif // wtk_lin_os
}

int64_t wtk_file_length(const WtkFile* file)
{
#if wtk_win_os
    LARGE_INTEGER size;
    if (!GetFileSizeEx(file->hnd, &size))
        return -1;

    return (int64_t)size.QuadPart;
#elif wtk_lin_os
    struct stat stat_res;
    if (fstat(file->fd, &stat_res) != 0)
        return -1;

    return (int64_t)stat_res.st_size;
#endif // wtk_lin_os

    return -1;
}

int64_t wtk_file_read_at(WtkFile* file, uint8_t* out_data, int64_t offset, int64_t length)
{
    if (offset < 0 || length < 0)
        return -1;

#if wtk_win_os
    // todo: from chatgpt. rewrite at some point.

    int64_t total = 0;
    while (total < length) {
        int64_t remaining = length - total;
        DWORD chunk = (remaining > (int64_t)MAXDWORD) ? MAXDWORD : (DWORD)remaining;

        int64_t pos = offset + total;
        OVERLAPPED ov = { 0 };
        ov.Offset = (DWORD)(pos & 0xFFFFFFFF);
        ov.OffsetHigh = (DWORD)((uint64_t)pos >> 32);

        DWORD read = 0;
        if (!ReadFile(file->hnd, out_data + total, chunk, &read, &ov)) {
            if (GetLastError() == ERROR_HANDLE_EOF)
                break; // offset at/after end of file

            return total > 0 ? total : -1;
        }

        if (read == 0)
            break; // EOF

        total += read;
        if (read < chunk)
            break; // short read
    }

    return total;
#elif wtk_lin_os
    // todo: our windows version above will continually read it
    // reads length bytes or EOF (rightfully so, since ReadFile
    // is capped to DWORD length.) however, pread does not
    // necessarily read length bytes. so now we need to match it
    // and have it loop as well.
    return (int64_t)pread(file->fd, out_data, (size_t)length, (off_t)offset);
#endif // wtk_lin_os

    return -1;
}

// path

char* wtk_file_path_filename(const char* path)
{
    const char* filename = path;
    for (const char* p = path; *p != '\0'; p++) {
        if (*p == '/' || *p == '\\')
            filename = p + 1;
    }

    return strdup(filename);
}