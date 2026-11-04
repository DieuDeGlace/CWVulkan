#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>

namespace captureabi {
static_assert(sizeof(std::uintptr_t) == 8, "The supported capture ABI is AMD64");

// Read copies exactly count bytes or fails. The caller supplies a guarded reader
// and the supported image/lifetime contract; this only checks address arithmetic.
template <class Read>
bool readField(std::uintptr_t object, std::size_t offset, void *destination, std::size_t count, Read &read) {
    constexpr auto maximum = std::numeric_limits<std::uintptr_t>::max();
    if (!object || offset > maximum - object)
        return false;
    const auto address = object + offset;
    return (!count || count - 1 <= maximum - address) && read(address, destination, count);
}
} // namespace captureabi
