#include "ChatCommands.h"
#include <cstring>
#include <sstream>
#include <utility>
namespace chatcommand {
Command parse(const std::u16string &text) {
    std::wistringstream stream(std::wstring(text.begin(), text.end()));
    std::wstring token;
    stream >> token;
    if (token == L"/check")
        return {Kind::Check};
    if (token == L"/namepet") {
        stream >> token;
        if (stream.fail())
            token.clear();
        return {Kind::NamePet, std::u16string(token.begin(), token.end())};
    }
    const std::pair<const wchar_t *, uint8_t> motions[] = {{L"/sit", 0x57}, {L"/wave", 0xa1}, {L"/pet", 0xa2}, {L"/dance", 0xa3}};
    for (const auto &[word, ability] : motions)
        if (token == word)
            return {Kind::Motion, {}, ability};
    return {}; // Caller sends the original text, with its whitespace intact.
}

void submit(const std::u16string &line,const Actions &actions) {
    const auto command=parse(line);
    switch (command.kind) {
    case Kind::Check:
        if (actions.toggleCheck) {
            const bool enabled=actions.toggleCheck();
            if (actions.feedback) actions.feedback(std::u16string(u"multiplayerWidget->checkGame = ")+(enabled ? u"1" : u"0"));
        }
        break;
    case Kind::NamePet:
        if (command.argument.size()>=16) {if (actions.feedback) actions.feedback(u"Name is too long.");}
        else if (!command.argument.empty() && actions.namePet) actions.namePet(command.argument);
        break;
    case Kind::Motion:
        if (actions.motion) actions.motion(command.ability);
        break;
    case Kind::Text:
        if (actions.text) actions.text(line);
        break;
    }
}
bool namePet(uint8_t *item, const std::u16string &name) {
    if (!item || item[0] != 20 || name.empty() || name.size() >= 16)
        return false;
    item[0x9d] = (uint8_t)name.size();
    for (size_t i = 0; i < name.size(); ++i)
        item[0x20 + 4 * i] = (uint8_t)name[i];
    return true; // Inactive attributes and all other bytes are retained.
}

bool startMotion(uint8_t *creature, float currentDuration, uint8_t ability) {
    if (!creature)
        return false;
    float clock;
    std::memcpy(&clock, creature + 0x6c, sizeof clock);
    if (!(clock > currentDuration))
        return false;
    creature[0x68] = ability;
    clock = 0;
    std::memcpy(creature + 0x6c, &clock, sizeof clock);
    return true;
}
} // namespace chatcommand
