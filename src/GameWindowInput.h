#pragma once
// Keyboard phases from the complete original-only Game/window contract.
// GUI code units, Game low-byte keys and physical action polling stay separate.
#include <array>
#include <cstdint>
#include <functional>

namespace gamewindow {
enum class Key { Down, Up, Character };
struct Keyboard {
    std::array<uint8_t,256> raw{};
    uint8_t menu = 0; // G+0x420's independently published menu byte
};
struct Target {
    Keyboard *keyboard = nullptr;
    std::function<void(uint8_t)> special; // F1..F6's synchronous effects
    std::function<void(uint8_t)> down; // current chat admission/handling after the switch
    std::function<void(uint8_t)> character; // Game performs its own sign extension
};
using Gui = std::function<void(Key,uint16_t)>;
using Current = std::function<Target()>;
void dispatch(Key,uint64_t wParam,const Gui &,const Current &);
float guiScale(float current,bool increase);
float mapScale(float current,bool increase);
}
