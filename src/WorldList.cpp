#include "WorldList.h"

#include <cstring>
#include <limits>
#include <sstream>

namespace worldlist {

bool online(const Entry &entry) { return entry.name.size() >= 7 && std::memcmp(entry.name.data(), "online_", 7) == 0; }

std::vector<int> cards(const std::vector<Entry> &list, bool multiplayer) {
    std::vector<int> indices;
    for (size_t i = 0; i < list.size(); ++i)
        if (online(list[i]) == multiplayer)
            indices.push_back(int(i));
    indices.push_back(int(list.size())); // New world, in both modes
    return indices;
}

bool createAdmitted(std::u16string_view name, std::u16string_view seed) {
    if (seed.empty() || name.size() < 2)
        return false;
    for (char16_t unit : seed)
        if (unit < u'0' || unit > u'9')
            return false;
    return true;
}

int32_t parseSeed(std::u16string_view digits) {
    int64_t value = 0;
    for (char16_t unit : digits) {
        value = value * 10 + (unit - u'0');
        if (value > std::numeric_limits<int32_t>::max())
            return std::numeric_limits<int32_t>::max();
    }
    return int32_t(value);
}

const char *selectCaption(int32_t current, size_t entries) {
    return int64_t(current) < int64_t(entries) ? "Select" : "Create";
}

const char *toggleCaption(bool multiplayer) { return multiplayer ? "Singleplayer worlds..." : "Multiplayer worlds..."; }

std::wstring exploredText(int32_t explored) {
    std::wostringstream text;
    text << L"Explored: " << float(explored) << L" km" << wchar_t(0x00b2); // superscript two
    return text.str();
}

std::wstring seedText(int32_t seed) { return L"Seed: " + std::to_wstring(seed); }

bool remove(std::vector<Entry> &list, int32_t &current) {
    if (current < 0 || size_t(current) >= list.size())
        return false;
    list.erase(list.begin() + current);
    current = -1;
    return true;
}

} // namespace worldlist
