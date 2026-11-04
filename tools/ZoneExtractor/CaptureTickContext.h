#pragma once
// Independent pointer/admission facts: steam-mod-loader-tick-context.md §§2–4.
#include <cstddef>
#include <cstdint>
#include <limits>

namespace captureabi {
constexpr std::uintptr_t CONTROLLER_SLOT_RVA = 0x551a80;
constexpr std::uintptr_t QUIT_BYTE_OFFSET = 0x1d68;

// Read must copy exactly the requested bytes or return false. The caller first
// validates the supported game image. Never dereference an unverified callback.
template <class Read> std::uintptr_t acceptedTickContext(std::uintptr_t module, std::uintptr_t reported, Read read) {
    static_assert(sizeof(std::uintptr_t) == 8, "The supported Steam loader ABI is AMD64");
    constexpr auto maximum = std::numeric_limits<std::uintptr_t>::max();
    if (!module || !reported || module > maximum - CONTROLLER_SLOT_RVA)
        return 0;
    std::uintptr_t controller = 0;
    if (!read(module + CONTROLLER_SLOT_RVA, &controller, sizeof controller) || controller != reported ||
        controller > maximum - QUIT_BYTE_OFFSET)
        return 0;
    std::uint8_t quitting = 1;
    if (!read(controller + QUIT_BYTE_OFFSET, &quitting, sizeof quitting) || quitting)
        return 0;
    return controller;
}
} // namespace captureabi
