#pragma once
// The world selection and world creation pages' rules, without drawing.
// docs/specs/steam-world-list-pages-and-world-entry-persistence-20261111.md
// In Steam both pages are unreachable from the title and character flow (H1) and the
// world-list store is never opened (H2): the list lives only for one session.
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace worldlist {

struct Entry {
    std::string name;     // entry+0x10, narrow
    int32_t seed = 0;     // entry+0x30
    int32_t explored = 0; // entry+0x34: zero at creation and never written again
};

// "online_" entries belong to the multiplayer list mode (contract 3.2 step 4a).
bool online(const Entry &);
// The list index behind each card, in card order: the entries of the current mode, then
// the New world card, whose index is the entry count (3.2 step 4).
std::vector<int> cards(const std::vector<Entry> &, bool multiplayer);
// Create World is shown when the seed is one or more of "0".."9" and the name has at
// least two UTF-16 units (4.2). There is no upper bound and no sign.
bool createAdmitted(std::u16string_view name, std::u16string_view seed);
// The seed from admitted digits (4.3 step 6). Values past INT32_MAX give INT32_MAX: the
// standard stream rule, an engine policy because the contract leaves overflow open (U2).
int32_t parseSeed(std::u16string_view digits);
// "Select" while the current index is below the entry count, else "Create" (3.4).
const char *selectCaption(int32_t current, size_t entries);
// The toggle's caption after a flip; construction uses "Multiplayer Worlds..." (3.7).
const char *toggleCaption(bool multiplayer);
inline constexpr const char *INITIAL_TOGGLE_CAPTION = "Multiplayer Worlds...";
// A card's two lower lines (3.5): the explored figure as a float in the default stream
// format with " km" and a superscript two, and the signed decimal seed.
std::wstring exploredText(int32_t explored);
std::wstring seedText(int32_t seed);
// Delete World (5.5): only for 0 <= current < count; the current index becomes -1.
bool remove(std::vector<Entry> &, int32_t &current);

} // namespace worldlist
