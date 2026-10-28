// RegionScout: CubeModLoader dev mod used to study Steam Cube World's
// landscapes for CubeVulkan. Read-only apart from moving the player.
//
//   * On the first tick in a world, asks the game's own region/biome functions
//     which landscape types exist within ~16k blocks and logs them, with the
//     nearest sample position of each, to Mods\RegionScout.log.
//   * F10 teleports the player to the nearest sample of the next landscape type.
//   * F9 returns the player to where they were before the first teleport.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "cwsdk.h"
#include <cmath>
#include <cstdio>
#include <map>

namespace scout {
constexpr uintptr_t REGION_AT_RVA = 0x2ada20;    // void (World*, IntVector2* out, x, y)
constexpr uintptr_t BIOME_AT_RVA = 0x2b9080;     // int (World*, regionX, regionY)
constexpr uintptr_t GROUND_HEIGHT_RVA = 0x35ea0; // float (World*, x, y, mode)
constexpr DWORD SUPPORTED_TIMESTAMP = 0x5d93ab0d;

const char *names[] = {"Plains",     "Ocean",     "Hills",    "Woodlands", "Jungle",
                       "Darkwoods",  "SnowForest", "Mountains", "Wetlands", "Desert",
                       "Savannah",   "Snowlands", "Deadlands", "Firelands"};

static uintptr_t base() { return (uintptr_t)GetModuleHandleW(nullptr); }

static bool supported() {
    auto b = (unsigned char *)base();
    auto nt = (IMAGE_NT_HEADERS64 *)(b + ((IMAGE_DOS_HEADER *)b)->e_lfanew);
    return nt->FileHeader.TimeDateStamp == SUPPORTED_TIMESTAMP;
}

static void log(const char *s) {
    FILE *f = nullptr;
    fopen_s(&f, "Mods\\RegionScout.log", "a");
    if (f) {
        fprintf(f, "%s\n", s);
        fclose(f);
    }
}

struct Sample {
    int64_t x = 0, y = 0;
    double distance = 0; // 0 until a suitable point is found.
    int regionX = 0, regionY = 0;
    int count = 0;
};
static std::map<int, Sample> found; // biome type -> nearest sample
static cube::World *scanned = nullptr;
static bool haveHome = false;
static LongVector3 home;
static int nextType = -1;
static bool f9 = false, f10 = false;

static int biomeAt(cube::World *w, int64_t x, int64_t y, IntVector2 &region) {
    ((void (*)(cube::World *, IntVector2 *, int, int))(base() + REGION_AT_RVA))(w, &region, (int)x, (int)y);
    return ((int (*)(cube::World *, int, int))(base() + BIOME_AT_RVA))(w, region.x, region.y);
}

static void scan(cube::World *w, cube::Creature *player) {
    found.clear();
    int64_t px = player->entity_data.position.x >> 16, py = player->entity_data.position.y >> 16;
    char s[256];
    sprintf_s(s, "Scan around %lld, %lld (seed %d)", (long long)px, (long long)py, w->seed);
    log(s);
    constexpr int STEP = 256, RANGE = 64; // 129 x 129 samples over +-16384 blocks.
    constexpr int N = 2 * RANGE + 1;
    static int types[N][N];
    for (int j = 0; j < N; j++)
        for (int i = 0; i < N; i++) {
            IntVector2 region{};
            types[j][i] = biomeAt(w, px + (int64_t)(i - RANGE) * STEP, py + (int64_t)(j - RANGE) * STEP, region);
        }
    auto ground = [&](int64_t x, int64_t y) {
        return ((float (*)(cube::World *, int, int, int))(base() + GROUND_HEIGHT_RVA))(w, (int)x, (int)y, 0);
    };
    for (int j = 1; j < N - 1; j++)
        for (int i = 1; i < N - 1; i++) {
            int type = types[j][i];
            int64_t x = px + (int64_t)(i - RANGE) * STEP, y = py + (int64_t)(j - RANGE) * STEP;
            found[type].count++;
            // Prefer points deep inside a region of this type and on dry land.
            bool interior = types[j - 1][i] == type && types[j + 1][i] == type && types[j][i - 1] == type &&
                            types[j][i + 1] == type;
            double d = std::sqrt(double((i - RANGE) * (i - RANGE) + (j - RANGE) * (j - RANGE))) * STEP;
            auto &s = found[type];
            if (!interior || (s.distance > 0 && d >= s.distance))
                continue;
            if (type != 1 && ground(x, y) < 4) // Ocean is allowed to be water.
                continue;
            IntVector2 region{};
            biomeAt(w, x, y, region);
            s.x = x;
            s.y = y;
            s.distance = d;
            s.regionX = region.x;
            s.regionY = region.y;
        }
    for (auto &[type, sample] : found) {
        sprintf_s(s, "  type %2d %-10s samples %5d  nearest at %lld, %lld (%.0f blocks, region %d,%d)", type,
                  type >= 0 && type < 14 ? names[type] : "?", sample.count, (long long)sample.x,
                  (long long)sample.y, sample.distance, sample.regionX, sample.regionY);
        log(s);
    }
}

static void moveTo(cube::World *w, cube::Creature *player, int64_t x, int64_t y) {
    float ground = ((float (*)(cube::World *, int, int, int))(base() + GROUND_HEIGHT_RVA))(w, (int)x, (int)y, 0);
    if (!std::isfinite(ground))
        ground = 100;
    player->entity_data.position.x = x << 16;
    player->entity_data.position.y = y << 16;
    // Drop in from a little above the ground (above water if it is lower).
    player->entity_data.position.z = (int64_t)((std::max(ground, 0.f) + 4) * 65536);
}

static void tick(cube::Game *g) {
    if (!g || !g->world || !g->world->local_creature || !supported())
        return;
    cube::World *w = g->world;
    cube::Creature *player = w->local_creature;
    if (scanned != w) {
        scanned = w;
        scan(w, player);
    }
    bool down10 = GetAsyncKeyState(VK_F10) & 0x8000, down9 = GetAsyncKeyState(VK_F9) & 0x8000;
    // F8: set the clock to noon (game time is milliseconds of a 24-hour day).
    static bool f8 = false;
    bool down8 = GetAsyncKeyState(VK_F8) & 0x8000;
    if (down8 && !f8) {
        // The host (server) world owns the clock in single player; set both.
        w->state.time = 12.f * 3600000.f;
        g->host.world.state.time = 12.f * 3600000.f;
        log("Clock set to 12:00");
    }
    f8 = down8;
    if (down10 && !f10 && !found.empty()) {
        if (!haveHome) {
            home = player->entity_data.position;
            haveHome = true;
        }
        auto it = found.upper_bound(nextType);
        if (it == found.end())
            it = found.begin();
        nextType = it->first;
        // Go a little into the region rather than onto its border.
        while (it->second.distance == 0 && std::next(it) != found.end()) // Skip types with no land point.
            it = std::next(it);
        nextType = it->first;
        moveTo(w, player, it->second.x, it->second.y);
        char s[160];
        sprintf_s(s, "Teleported to %s (type %d) at %lld, %lld", nextType < 14 ? names[nextType] : "?",
                  nextType, (long long)it->second.x, (long long)it->second.y);
        log(s);
    }
    if (down9 && !f9 && haveHome) {
        player->entity_data.position = home;
        log("Returned home");
    }
    f10 = down10;
    f9 = down9;
}
} // namespace scout

class RegionScout : public GenericMod {
  public:
    void Initialize() override { scout::log("RegionScout loaded (F10 next landscape, F9 home)"); }
    void OnGameTick(cube::Game *g) override { scout::tick(g); }
};
EXPORT GenericMod *MakeMod() { return new RegionScout(); }
EXPORT void ModPreInitialize() {}
EXPORT int ModMajorVersion() { return 7; }
EXPORT int ModMinorVersion() { return 3; }
