#pragma once
// Entered command actions from steam-chat-feat-presentation.md §§5.1/6.
#include <cstdint>
#include <functional>
#include <string>
namespace chatcommand {
enum class Kind { Text, Check, NamePet, Motion };
struct Command {
    Kind kind = Kind::Text;
    std::u16string argument;
    uint8_t ability = 0;
};
Command parse(const std::u16string &);
struct Actions {
    std::function<bool()> toggleCheck;
    std::function<void(const std::u16string &)> feedback, namePet, text;
    std::function<void(uint8_t)> motion;
};
void submit(const std::u16string &,const Actions &);
bool namePet(uint8_t *item, const std::u16string &name);
bool startMotion(uint8_t *creature, float currentDuration, uint8_t ability);
} // namespace chatcommand
