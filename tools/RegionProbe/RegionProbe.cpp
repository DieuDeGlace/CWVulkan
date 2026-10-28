// RegionProbe: read-only CubeModLoader dev mod. Studies how the running Steam
// game maps block positions to regions, for the BigRegions mod.
//   * At load: logs the in-memory bytes of the region/biome functions and every
//     direct call site of them in the game's code.
//   * First tick in a world: walks lines through the player's position and logs
//     where the region index changes, plus a coarse region grid.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "cwsdk.h"
#include <cstdio>
#include <string>
#include <vector>

namespace probe {
constexpr uintptr_t REGION_AT_RVA = 0x2ada20; // void (World*, IntVector2* out, x, y)
constexpr uintptr_t BIOME_AT_RVA = 0x2b9080;  // int (World*, regionX, regionY)
constexpr DWORD SUPPORTED_TIMESTAMP = 0x5d93ab0d;

static uintptr_t base() { return (uintptr_t)GetModuleHandleW(nullptr); }

static void log(const char *s) {
    FILE *f = nullptr;
    fopen_s(&f, "Mods\\RegionProbe.log", "a");
    if (f) {
        fprintf(f, "%s\n", s);
        fclose(f);
    }
}

static void dump(const char *name, uintptr_t rva, size_t n) {
    std::string line;
    char s[64];
    sprintf_s(s, "%s @ %llx:", name, (unsigned long long)rva);
    log(s);
    auto p = (unsigned char *)(base() + rva);
    for (size_t i = 0; i < n; i++) {
        if (i % 32 == 0) {
            if (!line.empty())
                log(line.c_str());
            sprintf_s(s, "%06llx ", (unsigned long long)(rva + i));
            line = s;
        }
        sprintf_s(s, "%02x", p[i]);
        line += s;
    }
    log(line.c_str());
}

static void callers(const char *name, uintptr_t target) {
    auto b = (unsigned char *)base();
    auto nt = (IMAGE_NT_HEADERS64 *)(b + ((IMAGE_DOS_HEADER *)b)->e_lfanew);
    auto sec = IMAGE_FIRST_SECTION(nt);
    std::string list;
    int count = 0;
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
        if (memcmp(sec->Name, ".text", 5) != 0)
            continue;
        uintptr_t start = sec->VirtualAddress, end = start + sec->Misc.VirtualSize;
        for (uintptr_t r = start; r + 5 <= end; r++) {
            unsigned char op = b[r];
            if (op != 0xe8 && op != 0xe9)
                continue;
            int32_t rel;
            memcpy(&rel, b + r + 1, 4);
            if (r + 5 + (int64_t)rel == target) {
                char s[32];
                sprintf_s(s, " %s%llx", op == 0xe9 ? "j" : "", (unsigned long long)r);
                list += s;
                count++;
            }
        }
    }
    char s[96];
    sprintf_s(s, "%s: %d direct call sites:", name, count);
    log(s);
    log(list.c_str());
}

static IntVector2 regionAt(cube::World *w, int64_t x, int64_t y) {
    IntVector2 r{};
    ((void (*)(cube::World *, IntVector2 *, int, int))(base() + REGION_AT_RVA))(w, &r, (int)x, (int)y);
    return r;
}
static int biomeAt(cube::World *w, IntVector2 r) {
    return ((int (*)(cube::World *, int, int))(base() + BIOME_AT_RVA))(w, r.x, r.y);
}

static bool scanned = false;

static void scan(cube::World *w, int64_t px, int64_t py) {
    char s[256];
    // The function takes int; positions are in dots (65536 per block) in the
    // entity, so try both block and dot interpretations.
    int64_t bx = px >> 16, by = py >> 16;
    sprintf_s(s, "Player block %lld, %lld (seed %d)", (long long)bx, (long long)by, w->seed);
    log(s);
    for (int axis = 0; axis < 2; axis++) {
        IntVector2 last = regionAt(w, bx, by);
        sprintf_s(s, "Walk along %s from %lld,%lld: start region %d,%d", axis ? "y" : "x", (long long)bx,
                  (long long)by, last.x, last.y);
        log(s);
        for (int64_t d = 1; d <= 40000; d += 4) {
            int64_t x = bx + (axis ? 0 : d), y = by + (axis ? d : 0);
            IntVector2 r = regionAt(w, x, y);
            if (r.x != last.x || r.y != last.y) {
                sprintf_s(s, "  at +%lld (%lld,%lld) -> region %d,%d biome %d", (long long)d, (long long)x,
                          (long long)y, r.x, r.y, biomeAt(w, r));
                log(s);
                last = r;
            }
        }
    }
    // Coarse map: 48x48 samples, 1024 blocks apart, region parity as letters.
    log("Region map (1024-block cells, letter = (rx*3+ry) mod 26, rows = y):");
    for (int j = -24; j < 24; j++) {
        std::string row;
        for (int i = -24; i < 24; i++) {
            IntVector2 r = regionAt(w, bx + i * 1024, by + j * 1024);
            row += (char)('a' + (((r.x * 3 + r.y) % 26) + 26) % 26);
        }
        log(row.c_str());
    }
    log("Biome map (same cells, hex biome):");
    for (int j = -24; j < 24; j++) {
        std::string row;
        for (int i = -24; i < 24; i++) {
            int b = biomeAt(w, regionAt(w, bx + i * 1024, by + j * 1024));
            row += "0123456789abcdef"[b & 15];
        }
        log(row.c_str());
    }
}

static void load() {
    auto b = (unsigned char *)base();
    auto nt = (IMAGE_NT_HEADERS64 *)(b + ((IMAGE_DOS_HEADER *)b)->e_lfanew);
    if (nt->FileHeader.TimeDateStamp != SUPPORTED_TIMESTAMP) {
        log("Unsupported executable.");
        return;
    }
    dump("REGION_AT", REGION_AT_RVA, 768);
    dump("BIOME_AT", BIOME_AT_RVA, 256);
    callers("REGION_AT", REGION_AT_RVA);
    callers("BIOME_AT", BIOME_AT_RVA);
}

static void tick(cube::Game *g) {
    if (scanned || !g || !g->world)
        return;
    auto player = g->world->local_creature;
    if (!player)
        return;
    scanned = true;
    scan(g->world, player->entity_data.position.x, player->entity_data.position.y);
    log("Scan done.");
}
} // namespace probe

class RegionProbe : public GenericMod {
  public:
    void Initialize() override {
        probe::log("RegionProbe loaded");
        probe::load();
    }
    void OnGameTick(cube::Game *g) override { probe::tick(g); }
};
EXPORT GenericMod *MakeMod() { return new RegionProbe(); }
EXPORT void ModPreInitialize() {}
EXPORT int ModMajorVersion() { return 7; }
EXPORT int ModMinorVersion() { return 3; }
