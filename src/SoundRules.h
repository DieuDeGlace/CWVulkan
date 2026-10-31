#pragma once
// The rules of the game's sound and music (docs/specs/sound.md), apart from the playing:
// how far a sound carries and how loud it is there, its pitch, which music track a place
// wants and when the music may change, what the world's ambient emitters play, and the wind
// while gliding. Audio (src/Audio.*) plays what these rules decide.
//
// Everything here is plain arithmetic on its inputs so the tests can check it without a
// sound device. Sounds are named by the game's sound numbers (§1.2); the file each number
// plays comes from the owner's install (tools/ZoneExtractor writes Extract/sounds.txt).
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace soundrules {

// --- Constants (§6) ---------------------------------------------------------------------------

constexpr float HEARING_RANGE_SQ = 4096.0f;  // blocks²: farther sounds are dropped (64 blocks)
constexpr float MAX_PITCH = 2.0f;            // the sound voices' largest frequency ratio
constexpr float OPTION_SCALE = 0.01f;        // the integer volume options are percent
constexpr float MUSIC_STOP_VOLUME = 0.01f;   // the music thread stops the music below this
constexpr float DUCKED_MUSIC_VOLUME = 0.015f;          // the music while an instrument plays
constexpr float DUCK_MS = 6000.0f;                     // how long one instrument note ducks the music
constexpr float SUCCESS_MS = 60000.0f;                 // how long the success track is wanted
constexpr float EXPLOSION_SHAKE_DISTANCE_SQ = 2500.0f; // blocks squared; zero shake at 50 blocks
constexpr float RAND_SCALE = 32767.0f;                 // the C runtime's rand() maximum

// Sound numbers the engine names (§2, §4, §5). The file names belong to the install.
namespace soundid {
constexpr int DUST = 0x01;
constexpr int DEATH = 0x1b;
constexpr int LEVEL_UP = 0x1d;
constexpr int QUEST_PROGRESS = 0x20;
constexpr int HUMAN_VOICE = 0x34;
constexpr int DROP_COIN = 0x3d;
constexpr int DROP_ITEM = 0x3e;
constexpr int EXPLOSION = 0x54;
constexpr int EXPLOSION_2 = 0x55;
constexpr int MENU_SELECT = 0x58;
constexpr int MENU_TAB = 0x59;
constexpr int CRAFT = 0x5c;
constexpr int CRAFT_SPECIAL = 0x5d;
constexpr int BIRD_FIRST = 0x61;   // 0x61..0x63, one per bird emitter type
constexpr int CRICKET_SMALL = 0x64;
constexpr int CRICKET_LARGE = 0x65;
constexpr int OWL_FIRST = 0x66;    // 0x66, 0x67
constexpr int RIVER_FIRST = 0x68;  // 0x68..0x6a
constexpr int INSTRUMENT_FIRST = 0x6d;
constexpr int INSTRUMENT_LAST = 0x77;
constexpr int WIND = 0x78;
} // namespace soundid

// --- Placing a sound (§2) ---------------------------------------------------------------------

// How a sound at squared distance d2 (blocks²) from the listener is heard: dropped beyond 64
// blocks, else its volume times 1 − d2 / 4096.
struct Heard {
    bool audible = false;
    float attenuation = 0;
};
Heard attenuate(float d2);
// The pitch a voice plays at: the voices cannot go above twice the speed (§1.3).
float cappedPitch(float pitch);
// The final volume of a sound (§2 step 6): the soundVolume option (percent) times the asked
// volume times the attenuation, as a fraction.
float finalVolume(int soundOption, float volume, float attenuation);
// Whether a sound number is an instrument's note, which ducks the music (§3.5).
bool isInstrument(int number);
// Native fixed differences narrow to float before conversion to blocks.
float distanceSquared(const int64_t a[3], const int64_t b[3]);
// Gameplay-camera §4: 1 - distanceSquared / 2500, not below zero.
float explosionShake(float distanceSquaredBlocks);
bool isExplosion(int number);

// --- The world's sound requests (§2.1; creature-hit.md §0.3) ---------------------------------

using RequestRecord = std::array<uint8_t, 0x30>;
namespace request {
constexpr size_t POSITION = 0x00; // i64 x, y, z (world units, 1/65536 block)
constexpr size_t NUMBER = 0x18;   // i32
constexpr size_t PITCH = 0x1c;    // f32
constexpr size_t VOLUME = 0x20;   // f32
constexpr size_t OWNER = 0x28;    // i64, not used for playing
} // namespace request
struct Request {
    int64_t position[3] = {0, 0, 0};
    int number = 0;
    float pitch = 1, volume = 1;
    int64_t owner = 0;
};
Request readRequest(const RequestRecord &record);
RequestRecord makeRequest(const Request &r);

// --- Music (§3) ---------------------------------------------------------------------------------

// What the music choice reads of the player's place (§3.3).
struct MusicPlace {
    int32_t cellX = 0, cellY = 0; // Z: the 1024-block place cell the player is in
    int biome = 0;               // B: the region's biome
    float ground = 0;            // the ground height at the player's column
    int placeKind = 0;           // K: the place kind, already 0 where its weight is under 0.5
    float forestWeight = 0;      // the forest weight at the column (decides kind 5)
    bool placeHashOdd = false;   // the low bit of the place cell's hash
    bool shrine = false;         // the block flag 0x20 at the player
    bool spiritWorld = false;    // the player has buff 0x13
    bool boss = false;           // a boss is after the player
    bool success = false;        // the success timer runs
    bool menu = false;           // a menu screen shows (character list, creation)
};
// A wanted track: its name (without .ogg), whether it is a place's own music (which may fade
// after five minutes), and the switch threshold a rule sets when it changes the track (ms;
// 0 for none).
struct Track {
    std::string name;
    bool placeMusic = false;
    float threshold = 0;
};
// The weight a place must have for its kind to count (§3.3).
constexpr float PLACE_WEIGHT_MIN = 0.5f;
// The track a place wants (§3.3). `current` is the track now chosen, so the rules that only
// set a threshold when they change the track can tell.
Track chooseTrack(const MusicPlace &place, const std::string &current);
// The music's state between frames (§3.4).
struct MusicState {
    std::string current;      // C: the track last requested
    float playedMs = 0;       // E: how long the current track has played
    float threshold = 300000; // T: the play time after which it fades out
    bool firstDifference = true; // F: the next frame wanting another track sets T
    float volume = 0;         // the music volume now (game +0x2048)
};
struct MusicOptions {
    int volume = 45;          // musicVolume, percent
    bool loop = false;        // musicLoop: place music never fades
};
// One frame of the music choice (§3.4): `playing` says whether the streamer is playing. Moves
// the state on and returns the path to request (Music\<current>.ogg) through state.current
// and the volume through state.volume.
void stepMusic(MusicState &state, const Track &wanted, bool playing, double dtMs, const MusicOptions &options,
               bool successRunning, float duckMs);
// Native frame smoothing; the rate depends on ducking, recovery and the wanted track.
// See remaining-runtime-gaps.md §8.1 for the verified coefficients and frame cap.
float easeMusicVolume(float volume, float target, double dtMs, float rate = 0.001f);

// --- Ambient emitters (§4) ----------------------------------------------------------------------

// An emitter as a zone keeps it (zone +0xa8, 0x40 bytes: type u32 at +0, position at +8).
struct Emitter {
    uint32_t type = 0;
    int64_t position[3] = {0, 0, 0};
};
namespace emitter {
constexpr size_t RECORD_SIZE = 0x40;
constexpr size_t TYPE = 0x00;
constexpr size_t POSITION = 0x08;
constexpr uint32_t WATER = 0x1f;
constexpr uint32_t SPARKLE = 0x4d; // a particle effect, no sound
constexpr uint32_t BIRD_FIRST = 0x4e; // 0x4e..0x50; 0x4e is an owl by night
constexpr uint32_t BIRD_LAST = 0x50;
} // namespace emitter
// The time of day bounds (ms, §4).
constexpr float NIGHT_END_MS = 21600000.0f;   // 6:00
constexpr float NIGHT_START_MS = 75600000.0f; // 21:00
constexpr float CRICKET_FROM_MS = 43200000.0f; // 12:00 (exclusive)
constexpr float CRICKET_TO_MS = 86400000.0f;   // 24:00 (exclusive)
bool isNight(float timeOfDayMs);
// The staggered tick (§4): an emitter whose offset is `offset` ticks on a period of `periodMs`
// when trunc((clock + offset) / period) changes over the frame.
bool staggeredTick(double clockMs, double dtMs, int64_t offset, int periodMs);
// The offset of an emitter at world position (x, y) for a period: (x × scale) in blocks and
// (y × scale + 0x14d0000) in blocks, each modulo the period, added. The water emitters use
// scale 13, the others 1.
int64_t tickOffset(int64_t x, int64_t y, int64_t scale, int periodMs);
// What one zone's emitters play this frame, in order (the 1-based index counts within the
// zone). `rand` is the C runtime's rand; `play` receives each sound to place.
struct AmbientFrame {
    double clockMs = 0;     // the world clock before this frame
    double dtMs = 0;        // the frame's ms
    float timeOfDayMs = 0;  // the time of day
};
using PlayAt = std::function<void(int number, const int64_t position[3], float volume, float pitch)>;
void ambientZone(const std::vector<Emitter> &emitters, const AmbientFrame &frame, const std::function<int()> &rand,
                 const PlayAt &play);

// --- The wind while gliding (§5.1) --------------------------------------------------------------

constexpr float WIND_START_MS = 500.0f; // the ability clock must pass this
constexpr float WIND_PERIOD_MS = 500.0f;
// Whether the glide's clock crossed a 500 ms boundary this frame, past the first half second.
bool windDue(float lastMs, float nowMs);
struct WindSound {
    float pitch = 1, volume = 1;
};
// Two rands: the pitch's (1..3 asked; heard at most 2) and then the volume's (0.5..1).
WindSound windSound(const std::function<int()> &rand);

// --- Ground items (§5.2) ------------------------------------------------------------------------

// The sounds an item makes when its pick-up delay runs out: coins (category 12) clink, other
// items drop; an item reserved for the local player of a category 3..9 or 0x1a also sounds
// the special craft chime. `rand` gives the pitch of the first.
struct LandingSound {
    int number = 0;
    float pitch = 1;
    bool chime = false; // also soundid::CRAFT_SPECIAL at pitch 1
};
LandingSound itemLanded(int category, bool reservedForLocalPlayer, const std::function<int()> &rand);

} // namespace soundrules
