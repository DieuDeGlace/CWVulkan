#pragma once
// Entered command actions from steam-chat-feat-presentation.md §§5.1/6.
#include <cstdint>
#include <string>
namespace chatcommand {
enum class Kind { Text, Check, NamePet, Motion };
struct Command {
    Kind kind = Kind::Text;
    std::u16string argument;
    uint8_t ability = 0;
};
Command parse(const std::u16string &);
bool namePet(uint8_t *item, const std::u16string &name);
bool startMotion(uint8_t *creature, float currentDuration, uint8_t ability);
} // namespace chatcommand
