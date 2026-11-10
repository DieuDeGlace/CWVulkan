#pragma once
// Selected early zoom and late menu consumers from the complete held sampled
// contract. Producers and gameplay/action owners retain separate state.
#include <array>
#include <cstdint>
#include <functional>

namespace sampledactions {
struct Process {
    std::array<uint8_t,2> zoom{};
    std::array<uint8_t,6> menus{}; // Native visit order15,17,18,16,20,19.
};
using Level = std::function<uint8_t(int action)>;
struct Menus {
    std::function<bool()> chatEditing;
    std::function<bool(int page)> pageShown; // Three direct roots, in their source order.
    Level level;
    std::function<void(int action)> invoke;
    std::function<void()> mirrorEquipment, mirrorDetails;
};
// False skips this block and ALL its retained stores; it does not skip a frame.
bool lateMenus(Process &, const Menus &);
// Native arithmetic precedes lower/upper correction. A zero count can yield -1.
void earlyZoom(Process &, const Level &, int32_t &index, const std::function<int32_t()> &count);
} // namespace sampledactions
