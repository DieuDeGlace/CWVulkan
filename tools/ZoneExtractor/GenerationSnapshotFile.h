#pragma once
// Exclusive publication and consumption of project diagnostic requests.
#ifdef _WIN32
#include "GenerationRequest.h"
#include <Windows.h>
#include <cstdint>
#include <cstdio>
#include <string>

namespace capturepolicy {
// Keep the exact input file exclusive through snapshot publication/consumption.
// A producer cannot replace its name after we read it and lose a newer request.
class GenerationRequestInput {
    HANDLE file = INVALID_HANDLE_VALUE;

  public:
    explicit GenerationRequestInput(const char *path)
        : file(CreateFileA(path, GENERIC_READ | DELETE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)) {}
    GenerationRequestInput(const GenerationRequestInput &) = delete;
    GenerationRequestInput &operator=(const GenerationRequestInput &) = delete;
    ~GenerationRequestInput() {
        if (file != INVALID_HANDLE_VALUE)
            CloseHandle(file);
    }
    explicit operator bool() const { return file != INVALID_HANDLE_VALUE; }
    // One bounded read. An extra byte lets the parser reject an oversized file.
    bool read(std::string &output) {
        char bytes[MaximumGenerationRequestBytes + 1];
        DWORD count = 0;
        if (!*this || !ReadFile(file, bytes, sizeof bytes, &count, nullptr))
            return false;
        output.assign(bytes, count);
        return true;
    }
    bool consume() {
        FILE_DISPOSITION_INFO disposition{TRUE};
        return *this && SetFileInformationByHandle(file, FileDispositionInfo, &disposition, sizeof disposition);
    }
};

// Legacy capture files keep their paths. Report errors from both buffered
// writes and close, which can fail while flushing data to the destination.
template <class Writer> bool writeGenerationArtifact(const std::string &path, Writer write) {
    std::FILE *file = nullptr;
    if (fopen_s(&file, path.c_str(), "wb") != 0 || !file)
        return false;
    const bool complete = write(file) && !std::ferror(file);
    const bool closed = std::fclose(file) == 0;
    return complete && closed;
}

inline bool publishGenerationSnapshot(const std::string &path, const std::string &text) {
    if (text.size() > UINT32_MAX)
        return false;
    const std::string temporary = path + ".tmp";
    HANDLE file = CreateFileA(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    DWORD written = 0;
    const bool complete = WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr) &&
                          written == text.size() && FlushFileBuffers(file);
    const bool closed = CloseHandle(file) != 0;
    // No replace flag: neither the temporary nor final owner file is overwritten.
    if (complete && closed && MoveFileExA(temporary.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH))
        return true;
    DeleteFileA(temporary.c_str());
    return false;
}
} // namespace capturepolicy
#endif
