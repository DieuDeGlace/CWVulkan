// The rules of the game's sound and music (docs/specs/sound.md). See SoundRules.h.
#include "SoundRules.h"
#include "GameMath.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace soundrules {

namespace {
// A rand() scaled to 0..k and moved by b, as the game's pitch rules write it.
float randomPitch(const std::function<int()> &rand, float k, float b) { return (float)rand() * k / RAND_SCALE + b; }
// "1 in n": one rand whose remainder by n is 0.
bool oneIn(const std::function<int()> &rand, int n) { return rand() % n == 0; }
} // namespace

// --- Placing a sound ----------------------------------------------------------------------------

Heard attenuate(float d2) {
    Heard h;
    if (d2 > HEARING_RANGE_SQ) // beyond 64 blocks the sound is dropped (0xa0420 step 2)
        return h;
    h.audible = true;
    h.attenuation = 1.0f - d2 * (1.0f / HEARING_RANGE_SQ);
    return h;
}

float cappedPitch(float pitch) { return std::min(pitch, MAX_PITCH); }

float finalVolume(int soundOption, float volume, float attenuation) {
    return (float)soundOption * volume * attenuation * OPTION_SCALE;
}

bool isInstrument(int number) {
    return number >= soundid::INSTRUMENT_FIRST && number <= soundid::INSTRUMENT_LAST;
}

bool isExplosion(int number) {
    return number == soundid::EXPLOSION || number == soundid::EXPLOSION_2;
}

float distanceSquared(const int64_t a[3], const int64_t b[3]) {
    return gamemath::fixedDistanceSquared(a, b);
}

float explosionShake(float distanceSquaredBlocks) {
    return std::max(0.0f, 1.0f - distanceSquaredBlocks / EXPLOSION_SHAKE_DISTANCE_SQ);
}

// --- Requests -----------------------------------------------------------------------------------

Request readRequest(const RequestRecord &record) {
    Request r;
    std::memcpy(r.position, record.data() + request::POSITION, sizeof r.position);
    int32_t number;
    std::memcpy(&number, record.data() + request::NUMBER, 4);
    r.number = number;
    std::memcpy(&r.pitch, record.data() + request::PITCH, 4);
    std::memcpy(&r.volume, record.data() + request::VOLUME, 4);
    std::memcpy(&r.owner, record.data() + request::OWNER, 8);
    return r;
}

RequestRecord makeRequest(const Request &r) {
    RequestRecord record{};
    std::memcpy(record.data() + request::POSITION, r.position, sizeof r.position);
    const int32_t number = r.number;
    std::memcpy(record.data() + request::NUMBER, &number, 4);
    std::memcpy(record.data() + request::PITCH, &r.pitch, 4);
    std::memcpy(record.data() + request::VOLUME, &r.volume, 4);
    std::memcpy(record.data() + request::OWNER, &r.owner, 8);
    return record;
}

// --- Music --------------------------------------------------------------------------------------

namespace {
constexpr float BEACH_BELOW = 10.0f; // ground height under which the coast plays
constexpr float OCEAN_BELOW = 0.0f;
constexpr float SPIRIT_THRESHOLD = 100.0f;
constexpr float QUICK_THRESHOLD = 500.0f;    // success and menus
constexpr float PLACE_FADE_MS = 300000.0f;   // five minutes
constexpr float SUCCESS_FADE_MS = 10000.0f;
constexpr float EARLIEST_CHANGE_MS = 30000.0f;
constexpr float LATE_TRACK_MS = 25000.0f;    // a track this far in waits 5 s more
constexpr float LATE_CHANGE_WAIT_MS = 5000.0f;
constexpr float URGENT_CHANGE_WAIT_MS = 500.0f; // boss and success

// The track of a region's biome (§3.3 step 3).
const char *biomeTrack(int biome) {
    switch (biome) {
    case 3: return "woodlands";
    case 4: return "jungle";
    case 5:
    case 12: return "deadlands";
    case 6:
    case 11: return "winter";
    case 7: return "mountains";
    case 9: return "desert";
    case 10: return "savannah";
    case 13: return "cult of doom";
    default: return "greenlands";
    }
}

// The track of a place kind, or null when the kind has none (§3.3 step 3).
const char *placeTrack(const MusicPlace &p) {
    switch (p.placeKind) {
    case 4:
    case 9: return "unholy pact";
    case 5: // a town: its forest weight decides (open question 7)
        if (p.forestWeight > PLACE_WEIGHT_MIN)
            return "unholy pact";
        return p.placeHashOdd ? "village2" : "village";
    case 8: return "order of the light";
    case 10: return "mountains";
    case 13:
    case 0x14:
    case 0x20: return "druids of mana";
    case 0x11: return "steel empire";
    case 0x12:
    case 0x25: return "cult of doom";
    case 0x19: return "woodlands";
    case 0x1a: return "deadlands";
    case 0x1b: return "enchanted-forest";
    default: return nullptr;
    }
}

bool urgentTrack(const std::string &name) { return name == "boss" || name == "success"; }
} // namespace

int32_t insidePlaceKind(int32_t zoneX, int32_t zoneY, int32_t kind, bool big) {
    if (!big)
        return 0;
    if (kind == 0x2a || (kind >= 0x2c && kind <= 0x2f))
        return kind;
    if (kind != 5 && kind != 9 && kind != 0xf)
        return 0;
    // The even zone's hash, in 32-bit wrapping arithmetic, then the image's exact high-product step.
    const uint32_t ex = (uint32_t)(zoneX & ~1), ey = (uint32_t)(zoneY & ~1);
    uint32_t u = ey * 0x39u + 0x62ad0u + ex;
    u ^= u << 13;
    const uint32_t v = ((u * u * 0xec4du + 0x131071fu) * u + 0xd208dd0du) & 0x7fffffffu;
    const int32_t high = (int32_t)(((int64_t)(int32_t)v * (int64_t)(int32_t)0x9e5706b3u) >> 32);
    const int32_t shifted = high >> 13;
    const int32_t q = shifted < 0 ? shifted + 1 : shifted;
    switch (kind) {
    case 5: return (q & 7) == 0 ? 0x29 : 0;
    case 9: return (q & 1) == 0 ? 0x28 : 0;
    default: return (q & 1) == 0 ? 0x2b : 0;
    }
}

int32_t musicZone(int64_t fixed) {
    const int32_t block = (int32_t)(fixed / 65536); // toward zero
    return block >> 10;                             // floored (arithmetic shift, C++20)
}

Track chooseTrack(const MusicPlace &place, const std::string &current) {
    Track t;
    if (place.cellX == 0 && place.cellY == 0 && place.placeKind != 0) {
        t.name = "maintheme"; // the title place
    } else if (place.shrine) {
        t.name = "shrine";
    } else {
        // The region, then the coast and the sea, then the place: all of it the place's own music.
        t.name = biomeTrack(place.biome);
        if (place.ground < BEACH_BELOW)
            t.name = "beach";
        if (place.ground < OCEAN_BELOW)
            t.name = "ocean";
        if (const char *own = placeTrack(place))
            t.name = own;
        t.placeMusic = true;
    }
    auto switchTo = [&](const char *name, float threshold) {
        if (current != name && threshold > 0)
            t.threshold = threshold;
        t.name = name;
        t.placeMusic = false;
    };
    if (place.spiritWorld)
        switchTo("deadlands", SPIRIT_THRESHOLD);
    if (place.boss)
        switchTo("boss", 0);
    if (place.success)
        switchTo("success", QUICK_THRESHOLD);
    if (place.menu)
        switchTo("maintheme", QUICK_THRESHOLD);
    return t;
}

float countDown(float timer, double ms) {
    return (float)std::max(0.0, std::trunc((double)timer - ms));
}

float easeMusicVolume(float volume, float target, double dtMs, float rate) {
    const float share = gamemath::blendFactor(std::min(dtMs, 1000.0), rate);
    return volume + (target - volume) * share;
}

void stepMusic(MusicState &s, const Track &wanted, bool playing, double dtMs, const MusicOptions &options,
               bool successRunning, float duckMs) {
    const bool differs = wanted.name != s.current;
    if (differs && wanted.threshold > 0)
        s.threshold = wanted.threshold;
    const float full = (float)options.volume * OPTION_SCALE;
    if (!playing) {
        // Silence: the wanted track starts at once, at full volume.
        if (differs)
            s.current = wanted.name;
        s.threshold = successRunning ? SUCCESS_FADE_MS : PLACE_FADE_MS;
        s.volume = full;
        s.playedMs = 0;
        s.firstDifference = true;
        return;
    }
    s.playedMs = (float)std::trunc((double)s.playedMs + dtMs); // whole ms (§4.4)
    if (!differs) {
        // Place music fades after five minutes unless the player loops it.
        s.threshold = wanted.placeMusic && !options.loop ? PLACE_FADE_MS : s.playedMs + PLACE_FADE_MS;
        s.firstDifference = true;
    } else if (s.firstDifference) {
        // Another track is wanted: the current one plays on for a while, then fades.
        if (urgentTrack(wanted.name))
            s.threshold = s.playedMs + URGENT_CHANGE_WAIT_MS;
        else if (s.playedMs < LATE_TRACK_MS)
            s.threshold = EARLIEST_CHANGE_MS;
        else
            s.threshold = s.playedMs + LATE_CHANGE_WAIT_MS;
        s.firstDifference = false;
    }
    float target, rate;
    if (duckMs > 0) {
        target = DUCKED_MUSIC_VOLUME;
        rate = 0.005f;
    } else if (s.playedMs <= s.threshold) {
        target = full;
        rate = 0.001f;
    } else {
        target = 0;
        rate = wanted.name == "boss" || successRunning ? 0.005f : 0.0004f;
    }
    s.volume = easeMusicVolume(s.volume, target, dtMs, rate);
}

// --- Ambient emitters ---------------------------------------------------------------------------

namespace {
constexpr int64_t UNITS_PER_BLOCK = 65536;
constexpr int64_t Y_STAGGER = 0x14d0000; // world units added to y before the offset
constexpr int WATER_PERIOD_MS = 500;
constexpr int64_t WATER_SCALE = 13;
constexpr int BIRD_PERIOD_MS = 1000;
constexpr int CRICKET_PERIOD_MS = 5000;
constexpr int WATER_CHANCE = 4, BIRD_CHANCE = 10, SPARKLE_CHANCE = 16, CRICKET_CHANCE = 70;
constexpr int CRICKET_EVERY = 20; // only every twentieth emitter of a zone chirps
constexpr float AMBIENT_VOLUME = 0.1f;
constexpr float CRICKET_VOLUME = 0.05f;

int32_t wholeMillis(float value) {
    // Match the instruction's signed-indefinite result outside the representable range.
    if (!(value >= -2147483648.0f && value < 2147483648.0f))
        return INT32_MIN;
    return int32_t(value);
}

int32_t blocksOf(int64_t units) { return std::bit_cast<int32_t>(uint32_t(units / UNITS_PER_BLOCK)); }

// The cricket an emitter type chirps with, 0 for none.
int cricketOf(uint32_t type) {
    if (type >= 4 && type <= 8)
        return soundid::CRICKET_SMALL;
    if (type == 0 || type == 1 || type == 11 || type == 12)
        return soundid::CRICKET_LARGE;
    return 0;
}
} // namespace

bool isNight(float t) { return t <= NIGHT_END_MS || t >= NIGHT_START_MS; }

AmbientFrame ambientFrame(float worldClockMs, double frameMs, float timeOfDayMs) {
    return {wholeMillis(worldClockMs), int32_t(gamemath::frameMilliseconds(frameMs)), timeOfDayMs};
}

bool staggeredTick(int32_t clockMs, int32_t dtMs, int32_t offset, int32_t periodMs) {
    const int32_t before = gamemath::add32(clockMs, offset);
    const int32_t after = gamemath::add32(before, dtMs);
    return before / periodMs != after / periodMs;
}

int32_t tickOffset(int64_t x, int64_t y, int64_t scale, int32_t divisor) {
    return blocksOf(gamemath::multiplyFixed(x, scale)) % divisor +
           blocksOf(gamemath::addFixed(gamemath::multiplyFixed(y, scale), Y_STAGGER)) % divisor;
}

void ambientZone(std::span<const Emitter> emitters, const AmbientFrame &frame, const std::function<int()> &rand,
                 const PlayAt &play, const SpawnLeaf &leaf) {
    const bool daylight = frame.timeOfDayMs > NIGHT_END_MS && frame.timeOfDayMs < NIGHT_START_MS;
    const bool crickets = frame.timeOfDayMs > CRICKET_FROM_MS && frame.timeOfDayMs < CRICKET_TO_MS;
    for (size_t k = 0; k < emitters.size(); ++k) {
        const Emitter &e = emitters[k];
        const auto ticks = [&](int32_t period, int32_t offset) {
            return staggeredTick(frame.clockMs, frame.dtMs, offset, period);
        };
        if (e.type == emitter::WATER) {
            if (ticks(WATER_PERIOD_MS, tickOffset(e.position[0], e.position[1], WATER_SCALE, BIRD_PERIOD_MS)) &&
                oneIn(rand, WATER_CHANCE)) {
                const float pitch = randomPitch(rand, 0.4f, 0.8f);
                play(soundid::RIVER_FIRST + rand() % 3, e.position, AMBIENT_VOLUME, pitch);
            }
            continue;
        }
        if (e.type == emitter::SPARKLE) {
            if (ticks(BIRD_PERIOD_MS, 0) && oneIn(rand, SPARKLE_CHANCE)) {
                const auto draw = [&](float scale) { return (float(rand()) / RAND_SCALE - 0.5f) * scale; };
                const float vy = draw(4.0f);
                const float vx = draw(4.0f);
                const float az = draw(10.0f);
                const float ay = draw(10.0f);
                const float ax = draw(10.0f);
                if (leaf)
                    leaf({{e.position[0], e.position[1], e.position[2]}, {vx, vy, -2.0f}, {ax, ay, az}, e.colour});
            }
            continue;
        }
        if (e.type >= emitter::BIRD_FIRST && e.type <= emitter::BIRD_LAST) {
            if ((daylight || e.type == emitter::BIRD_FIRST) &&
                ticks(BIRD_PERIOD_MS, tickOffset(e.position[0], e.position[1], 1, BIRD_PERIOD_MS)) && oneIn(rand, BIRD_CHANCE)) {
                const float pitch = randomPitch(rand, 0.2f, daylight ? 0.9f : 0.5f);
                const int sound = daylight ? soundid::BIRD_FIRST + int(e.type - emitter::BIRD_FIRST) : soundid::OWL_FIRST + rand() % 2;
                play(sound, e.position, AMBIENT_VOLUME, pitch);
            }
            continue;
        }
        if (!crickets || (k + 1) % CRICKET_EVERY != 0)
            continue;
        const int cricket = cricketOf(e.type);
        if (cricket && ticks(CRICKET_PERIOD_MS, tickOffset(e.position[0], e.position[1], 1, CRICKET_PERIOD_MS)) &&
            oneIn(rand, CRICKET_CHANCE))
            play(cricket, e.position, CRICKET_VOLUME, randomPitch(rand, 0.2f, 0.9f));
    }
}

// --- Wind ---------------------------------------------------------------------------------------

bool windDue(float lastMs, float nowMs) {
    return nowMs > WIND_START_MS && (int)(lastMs / WIND_PERIOD_MS) != (int)(nowMs / WIND_PERIOD_MS);
}

WindSound windSound(const std::function<int()> &rand) {
    const int forPitch = rand();
    const int forVolume = rand();
    WindSound w;
    w.pitch = (float)(forPitch + forPitch) / RAND_SCALE + 1.0f;
    w.volume = (float)forVolume * 0.5f / RAND_SCALE + 0.5f;
    return w;
}

// --- Ground items -------------------------------------------------------------------------------

LandingSound itemLanded(int category, bool reservedForLocalPlayer, const std::function<int()> &rand) {
    constexpr int COINS = 12;
    LandingSound s;
    s.number = category == COINS ? soundid::DROP_COIN : soundid::DROP_ITEM;
    s.pitch = randomPitch(rand, 0.1f, 1.0f);
    s.chime = reservedForLocalPlayer && ((category >= 3 && category <= 9) || category == 0x1a);
    return s;
}

} // namespace soundrules
