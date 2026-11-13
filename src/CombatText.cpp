#include "CombatText.h"
#include <cmath>
#include <cstring>
#include <limits>
#include <locale>
#include <sstream>
namespace combattext {
namespace {
constexpr size_t ATTACKER = 0x00, TARGET = 0x08, AMOUNT = 0x10, CRITICAL = 0x14, STUN = 0x18, POSITION = 0x20,
                 SPIRIT = 0x44, KIND = 0x45, SIZE = 0x8c, DEALT = 0xbc0, DEALT_RESET = 0xbc8;
constexpr int64_t BLOCK = 65536;
template <class T> T get(const uint8_t *p, size_t at) {
    T v;
    std::memcpy(&v, p + at, sizeof v);
    return v;
}
template <class T> void put(uint8_t *p, size_t at, T v) { std::memcpy(p + at, &v, sizeof v); }
// CVTTSS2SI: out-of-range and NaN give the integer-indefinite value.
int32_t truncate(float v) {
    if (!(v > -2147483904.0f && v < 2147483648.0f))
        return std::numeric_limits<int32_t>::min();
    return (int32_t)v;
}
int64_t truncate64(float v) {
    if (!(v > -9.2233725e18f && v < 9.2233720e18f))
        return std::numeric_limits<int64_t>::min();
    return (int64_t)v;
}
} // namespace

std::wstring number(float amount) {
    const float v = std::fabs(amount);
    std::wostringstream out;
    out.imbue(std::locale::classic());
    if (v >= 1.0f || std::isnan(v))
        out << truncate(v);
    else
        out << (float)truncate(v * 10.0f) * 0.1f;
    return out.str();
}

std::vector<feattext::Record> visit(const Hit &hit, const Lookup &lookup, const uint8_t *player) {
    std::vector<feattext::Record> texts;
    const uint8_t *h = hit.data();
    uint8_t *target = lookup(get<int64_t>(h, TARGET));
    if (!target) // the record gate: an unknown target yields nothing at all
        return texts;
    const float amount = get<float>(h, AMOUNT);
    uint8_t *attacker = lookup(get<int64_t>(h, ATTACKER));
    if (attacker && amount >= 0.0f) { // 6.1
        put<float>(attacker, DEALT, get<float>(attacker, DEALT) + amount);
        put<int32_t>(attacker, DEALT_RESET, 0);
    }
    const bool onPlayer = player && target == player;
    feattext::Record base;
    base.anchor = {get<int64_t>(h, POSITION), get<int64_t>(h, POSITION + 8), get<int64_t>(h, POSITION + 16) + 2 * BLOCK};
    base.offsetX = 0, base.offsetY = 0, base.lifetime = LIFETIME, base.fontSize = FONT_SIZE, base.flags = FLAGS;
    if (get<int32_t>(h, STUN) != 0) { // 3.2
        feattext::Record ko = base;
        ko.text = L"K.O.", ko.colour = {1, 1, 0, 1}, ko.drift = onPlayer ? -50.0f : 50.0f;
        texts.push_back(std::move(ko));
    }
    const uint8_t kind = h[KIND];
    const bool involved = player && (attacker == player || onPlayer);
    if (!involved || ((uint8_t)(kind - 4) & ~2) == 0 || (kind == 0 && !(std::fabs(amount) >= 0.1f)))
        return texts;
    feattext::Record text = base; // 3.3
    const float size = (1.0f - get<float>(target, SIZE) * 0.5f) * 65536.0f;
    text.anchor[2] = (int64_t)((uint64_t)text.anchor[2] + (uint64_t)truncate64(size));
    const bool critical = h[CRITICAL] != 0;
    text.fontSize = critical ? CRITICAL_FONT_SIZE : FONT_SIZE;
    if (amount < 0.0f)
        text.colour = {0.4f, 1, 0.5f, 1}, text.drift = onPlayer ? 100.0f : -100.0f;
    else if (onPlayer)
        text.colour = {1, 0.3f, 0.3f, 1}, text.drift = 100.0f;
    else
        text.colour = {1, 1, h[SPIRIT] ? 0.25f : 1.0f, 1}, text.drift = critical ? -200.0f : -100.0f;
    if (onPlayer)
        text.anchor[2] -= BLOCK;
    text.text = kind == 1 ? L"BLOCK\n" : kind == 3 ? L"MISS\n" : kind == 5 ? L"ABSORB\n" : number(amount);
    texts.push_back(std::move(text));
    return texts;
}
} // namespace combattext
