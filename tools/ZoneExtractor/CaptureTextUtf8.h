#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <windows.h>

namespace captureabi {
// Policy for the existing NUL-delimited line export, not a native string rule.
// Reject unrepresentable data instead of silently replacing or truncating it.
inline bool exportUtf8(std::u16string_view text, std::string &output) {
    output.clear();
    if (text.empty())
        return true;
    if (text.size() > size_t(std::numeric_limits<int>::max()) || text.find(u'\0') != std::u16string_view::npos)
        return false;
    static_assert(sizeof(wchar_t) == sizeof(char16_t));
    const auto *units = reinterpret_cast<const wchar_t *>(text.data());
    const int length = static_cast<int>(text.size());
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, units, length, nullptr, 0, nullptr, nullptr);
    if (count <= 0)
        return false;
    std::string converted(count, '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, units, length, converted.data(), count, nullptr, nullptr) !=
        count)
        return false;
    output = std::move(converted);
    return true;
}
} // namespace captureabi
