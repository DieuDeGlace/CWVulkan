#pragma once
// Outside Windows: the durable replacement the Windows build makes with CreateFileW(CREATE_NEW),
// FlushFileBuffers and MoveFileExW(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH). A new
// temporary file beside the target is written, flushed to storage and renamed over the target,
// so a reader sees the old file or the new one, never part of one.
#ifndef _WIN32
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <string>

#include <fcntl.h>
#include <unistd.h>

namespace posixfile {
inline bool replace(const std::filesystem::path &target, const void *data, size_t size) {
    static std::atomic<unsigned> serial{0};
    std::filesystem::path temporary = target;
    temporary += ".tmp-" + std::to_string(getpid()) + "-" + std::to_string(serial++);
    const int file = open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0644);
    if (file < 0)
        return false;
    bool ok = true;
    for (size_t done = 0; ok && done < size;) {
        const ssize_t n = write(file, static_cast<const char *>(data) + done, size - done);
        ok = n > 0;
        done += ok ? size_t(n) : 0;
    }
    ok = fsync(file) == 0 && ok;
    ok = close(file) == 0 && ok;
    if (ok)
        ok = std::rename(temporary.c_str(), target.c_str()) == 0;
    if (!ok) {
        unlink(temporary.c_str());
        return false;
    }
    // The rename itself is durable once the folder is flushed.
    const auto folder = target.parent_path().empty() ? std::filesystem::path(".") : target.parent_path();
    if (const int dir = open(folder.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC); dir >= 0) {
        fsync(dir);
        close(dir);
    }
    return true;
}
} // namespace posixfile
#endif
