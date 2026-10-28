// ZoneExtractor: CubeModLoader mod that saves the zones Steam Cube World
// generates (every block's exact colour and type: terrain, trees, buildings)
// so CubeVulkan can render the identical world.
//
// Output: <game>\Extract\seed_<seed>\
//   manifest.txt           seed and the home position (blocks)
//   z_<x>_<y>.cwz          one file per zone (format below)
//
// Native oracle: create <game>\Extract\oracle.request while in a world and the next tick
// writes Extract\oracle\ (the creature tables and test samples; namespace oracle below).
//
// Sound list: Extract\sounds.txt (the game's sound numbers and their files) is written the
// first time a world is entered without it, and again on Extract\sounds.request.
//
// Packet recorder: Extract\net.request records the multiplayer packets into the folder
// Extract\net (namespace netrecorder below).
//
// Keys: F11 starts/stops a sweep that visits a grid around the home position
// (the character is held in place with full health while sweeping); F9 returns
// home. Zones are also saved as you play normally.
//
// .cwz format (little endian):
//   char[4] "CWZ1"; int32 zoneX, zoneY, regionType, reserved;
//   4096 columns in the game's order (index = localX * 64 + localY):
//     int32 baseZ; uint32 runs; runs x { uint16 count; uint8 block[6] }
//   block = red, green, blue, field_3, type, breakable (cube::Block bytes).
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "cwsdk.h"
#include <dxgi1_2.h>
#include <intrin.h>
#include <mmsystem.h>
#include <tlhelp32.h>
#pragma comment(lib, "winmm.lib")
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <share.h>
#include <deque>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace extract {
constexpr uintptr_t REGION_AT_RVA = 0x2ada20;    // void (World*, IntVector2* out, x, y)
constexpr uintptr_t BIOME_AT_RVA = 0x2b9080;     // int (World*, regionX, regionY)
constexpr uintptr_t GROUND_HEIGHT_RVA = 0x35ea0; // float (World*, x, y, mode)
constexpr uintptr_t MAX_HP_RVA = 0x5fac0;        // float (Creature*)
constexpr DWORD SUPPORTED_TIMESTAMP = 0x5d93ab0d;
constexpr int SWEEP_RADIUS = 8;   // Stops in each direction (17 x 17 stops, about 6,500 blocks across).
constexpr int SWEEP_STEP = 6;     // Zones between stops.
constexpr int SETTLE_RADIUS = 3;  // Zones around a stop that must be saved before moving on.
constexpr ULONGLONG STOP_TIMEOUT_MS = 20000;

static uintptr_t base() { return (uintptr_t)GetModuleHandleW(nullptr); }
static bool supported() {
    auto b = (unsigned char *)base();
    auto nt = (IMAGE_NT_HEADERS64 *)(b + ((IMAGE_DOS_HEADER *)b)->e_lfanew);
    return nt->FileHeader.TimeDateStamp == SUPPORTED_TIMESTAMP;
}
static void log(const std::string &s) {
    FILE *f = nullptr;
    fopen_s(&f, "Mods\\ZoneExtractor.log", "a");
    if (f) {
        fprintf(f, "%s\n", s.c_str());
        fclose(f);
    }
}
static int64_t floorDiv(int64_t a, int64_t b) { return a / b - ((a % b != 0) && ((a < 0) != (b < 0))); }

// Files are written on a worker thread so the game never waits on the disk.
struct Job {
    std::string path;
    std::vector<unsigned char> data;
};
static std::deque<Job> jobs;
static std::mutex jobsMutex;
static std::condition_variable jobsReady;
static void writer() {
    for (;;) {
        Job job;
        {
            std::unique_lock lock(jobsMutex);
            jobsReady.wait(lock, [] { return !jobs.empty(); });
            job = std::move(jobs.front());
            jobs.pop_front();
        }
        FILE *f = nullptr;
        if (fopen_s(&f, job.path.c_str(), "wb") == 0 && f) {
            fwrite(job.data.data(), 1, job.data.size(), f);
            fclose(f);
        }
    }
}

// --- Sprite names -----------------------------------------------------------
// The game loads every sprite with ParseCubToSprite(Sprite*, std::string file,
// ...) from its own internal list. A one-byte breakpoint on that function,
// handled by a vectored exception handler, records which file each Sprite
// object came from; the byte is restored around each call (single step).
constexpr uintptr_t PARSE_CUB_RVA = 0x16F320;
static unsigned char *parseEntry = nullptr;
static unsigned char parseByte = 0;
static std::mutex namesMutex;
static std::vector<std::pair<void *, std::string>> spriteNames;
static bool namesWritten = false;
static volatile bool armed = false; // True while the breakpoint is in use.

static std::string readMsvcString(const unsigned char *s) {
    // MSVC std::string: 16-byte inline buffer or pointer, then size, capacity.
    size_t size, capacity;
    memcpy(&size, s + 16, 8);
    memcpy(&capacity, s + 24, 8);
    if (size > 4096)
        return {};
    const char *data = capacity > 15 ? *(const char *const *)s : (const char *)s;
    return std::string(data, size);
}

static LONG CALLBACK breakpoint(EXCEPTION_POINTERS *e) {
    DWORD code = e->ExceptionRecord->ExceptionCode;
    if (code == EXCEPTION_BREAKPOINT && armed && e->ExceptionRecord->ExceptionAddress == parseEntry) {
        {
            std::lock_guard lock(namesMutex);
            spriteNames.push_back({(void *)e->ContextRecord->Rcx,
                                   readMsvcString((const unsigned char *)e->ContextRecord->Rdx)});
        }
        *parseEntry = parseByte; // Run the original instruction once...
        e->ContextRecord->Rip = (DWORD64)parseEntry;
        e->ContextRecord->EFlags |= 0x100; // ...stepping over it,
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (code == EXCEPTION_SINGLE_STEP && armed && parseEntry && *parseEntry != 0xCC) {
        *parseEntry = 0xCC; // ...then re-arm the breakpoint.
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static void hookSpriteLoading() {
    if (!supported())
        return;
    parseEntry = (unsigned char *)(base() + PARSE_CUB_RVA);
    DWORD old;
    // The code page stays writable so the handler can swap the byte.
    if (!VirtualProtect(parseEntry, 1, PAGE_EXECUTE_READWRITE, &old))
        return;
    parseByte = *parseEntry;
    AddVectoredExceptionHandler(1, breakpoint);
    armed = true;
    *parseEntry = 0xCC;
    FlushInstructionCache(GetCurrentProcess(), parseEntry, 1);
}

// After loading, the breakpoint is removed and the slot -> file table written.
static void writeSpriteTable(cube::World *w) {
    if (namesWritten || !w->spritemanager || w->spritemanager->sprites.empty())
        return;
    std::vector<std::pair<void *, std::string>> names;
    {
        std::lock_guard lock(namesMutex);
        names = spriteNames;
    }
    armed = false;
    if (parseEntry && *parseEntry == 0xCC) {
        *parseEntry = parseByte;
        FlushInstructionCache(GetCurrentProcess(), parseEntry, 1);
    }
    std::string path = "Extract\\sprites.txt";
    FILE *f = nullptr;
    if (fopen_s(&f, path.c_str(), "w") == 0 && f) {
        const auto &sprites = w->spritemanager->sprites;
        int named = 0;
        for (size_t i = 0; i < sprites.size(); i++) {
            auto s = sprites[i];
            std::string name;
            for (auto it = names.rbegin(); it != names.rend(); ++it)
                if (it->first == s) {
                    name = it->second;
                    break;
                }
            named += !name.empty();
            fprintf(f, "%zu\t%s\t%d\t%d\t%d\n", i, name.empty() ? "?" : name.c_str(), s ? s->something_with_offset_1 : 0,
                    s ? s->something_with_offset_2 : 0, s ? s->lowering_this_makes_plants_go_fast : 0);
        }
        fclose(f);
        log("Sprite table: " + std::to_string(sprites.size()) + " slots, " + std::to_string(named) + " named, " +
            std::to_string(names.size()) + " loads seen");
    }
    // The syllables of the game's generated names (randomName, 0x611d0): the function loads
    // its 380 string literals in table order (the default first and second halves, 30 each,
    // then the races' halves, 10 each), each by "lea rdx, [rip+literal]". They are read
    // from the running game's own code here, so CubeVulkan carries no copy of them.
    if (fopen_s(&f, "Extract\\names.txt", "w") == 0 && f) {
        const unsigned char *code = (const unsigned char *)base();
        int count = 0;
        for (uintptr_t rva = 0x611d0; rva + 7 <= 0x64565; rva++)
            if (code[rva] == 0x48 && code[rva + 1] == 0x8d && code[rva + 2] == 0x15) {
                int32_t displacement;
                memcpy(&displacement, code + rva + 3, 4);
                const uintptr_t target = rva + 7 + (intptr_t)displacement;
                if (target < 0x430000 || target >= 0x440000) // (its string literals sit in .rdata there)
                    continue;
                const wchar_t *s = (const wchar_t *)(code + target);
                std::string text;
                for (int i = 0; i < 32 && s[i]; i++)
                    text.push_back((char)s[i]);
                fprintf(f, "%s\n", text.c_str());
                count++;
                rva += 6;
            }
        fclose(f);
        log("Name syllables: " + std::to_string(count) + " written to Extract\\names.txt");
    }
    // Environment object types: SpriteManager+0x28 is a std::map<uint8_t,
    // std::vector<Sprite*>> (type -> variant sprites); +0x38/+0x50/+0x68 are
    // the environment, building-part and cage sprite vectors.
    auto nameOf = [&](void *s) -> std::string {
        for (auto it = names.rbegin(); it != names.rend(); ++it)
            if (it->first == s)
                return it->second;
        return "?";
    };
    if (fopen_s(&f, "Extract\\sprite-groups.txt", "w") == 0 && f) {
        auto manager = (unsigned char *)w->spritemanager;
        struct Node {
            Node *left, *parent, *right;
            char color, isnil, pad[6];
            uint8_t key, pad2[7];
            void **begin, **end, **capacity;
        };
        Node *head = *(Node **)(manager + 0x28);
        size_t size = *(size_t *)(manager + 0x30);
        fprintf(f, "# type map (%zu entries)\n", size);
        // In-order walk of the red-black tree.
        std::vector<Node *> stack;
        Node *n = head ? head->parent : nullptr; // Root.
        size_t visited = 0;
        while ((n && !n->isnil) || !stack.empty()) {
            while (n && !n->isnil && stack.size() < 64) {
                stack.push_back(n);
                n = n->left;
            }
            n = stack.back();
            stack.pop_back();
            fprintf(f, "type %u:", n->key);
            for (void **s = n->begin; s && s < n->end && s - n->begin < 256; s++)
                fprintf(f, " %s", nameOf(*s).c_str());
            fprintf(f, "\n");
            if (++visited > 1024)
                break;
            n = n->right;
        }
        const char *vectors[] = {"environment", "building-part", "cage"};
        for (int v = 0; v < 3; v++) {
            void ***vec = (void ***)(manager + 0x38 + v * 0x18);
            fprintf(f, "# %s sprites\n", vectors[v]);
            for (void **s = vec[0]; s && s < vec[1] && s - vec[0] < 4096; s++)
                fprintf(f, "%zd %s\n", s - vec[0], nameOf(*s).c_str());
        }
        fclose(f);
    }
    namesWritten = true;
}

// --- Sprite scale calibration -------------------------------------------------
// Each object type is drawn at its own size, set in the game's code. The
// D3D11 draws reveal it: a sprite draw binds the sprite's vertex buffer and
// its world matrix (vertex-shader constant buffer b2, as SteamShadows uses).
// The matrix's basis length, relative to a terrain chunk's (one block per
// voxel), is the sprite's size in blocks per voxel.
namespace calibration {
constexpr UINT SLOT_DRAW_INDEXED = 12, SLOT_MAP = 14, SLOT_UNMAP = 15, SLOT_UPDATE = 48;
using DrawIndexed = void(STDMETHODCALLTYPE *)(ID3D11DeviceContext *, UINT, UINT, INT);
using MapFn = HRESULT(STDMETHODCALLTYPE *)(ID3D11DeviceContext *, ID3D11Resource *, UINT, D3D11_MAP, UINT,
                                           D3D11_MAPPED_SUBRESOURCE *);
using UnmapFn = void(STDMETHODCALLTYPE *)(ID3D11DeviceContext *, ID3D11Resource *, UINT);
using UpdateFn = void(STDMETHODCALLTYPE *)(ID3D11DeviceContext *, ID3D11Resource *, UINT, const D3D11_BOX *,
                                           const void *, UINT, UINT);
static std::mutex dataMutex;
static std::unordered_map<ID3D11Resource *, void *> mapped;              // CB -> mapped pointer
static std::unordered_map<ID3D11Resource *, std::array<float, 16>> matrices; // CB -> last 64 bytes
static std::unordered_map<ID3D11Buffer *, int> spriteOf;                  // VB -> sprite slot
static std::unordered_set<ID3D11Buffer *> chunks;                         // terrain chunk VBs
static std::unordered_map<int, std::vector<float>> samples;               // slot -> basis lengths
static std::vector<float> chunkSamples;

static bool smallConstantBuffer(ID3D11Resource *r) {
    D3D11_RESOURCE_DIMENSION d{};
    r->GetType(&d);
    if (d != D3D11_RESOURCE_DIMENSION_BUFFER)
        return false;
    D3D11_BUFFER_DESC desc{};
    static_cast<ID3D11Buffer *>(r)->GetDesc(&desc);
    return (desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER) && desc.ByteWidth == 64;
}
// Diagnostics: draws seen, draws whose b2 matrix was known, sprite and chunk matches.
static std::atomic<uint64_t> drawsSeen{0}, drawsWithMatrix{0}, spriteDraws{0}, chunkDraws{0};
using DrawFn = void(STDMETHODCALLTYPE *)(ID3D11DeviceContext *, UINT, UINT);
using DrawIndexedInstancedFn = void(STDMETHODCALLTYPE *)(ID3D11DeviceContext *, UINT, UINT, UINT, INT, UINT);
using DrawInstancedFn = void(STDMETHODCALLTYPE *)(ID3D11DeviceContext *, UINT, UINT, UINT, UINT);
constexpr UINT SLOT_DRAW_INDEXED_INSTANCED = 20, SLOT_DRAW_INSTANCED = 21;
// The game uses more than one device context (UI previews and the world), and
// their classes can have different vtables. Each hooked vtable keeps its own
// original functions.
struct Originals {
    void **table;
    DrawIndexed drawIndexed;
    DrawFn draw;
    MapFn map;
    UnmapFn unmap;
    UpdateFn update;
    DrawIndexedInstancedFn drawIndexedInstanced;
    DrawInstancedFn drawInstanced;
};
static Originals originals[8];
static int originalCount = 0;
static const Originals &of(ID3D11DeviceContext *c) {
    void **t = *(void ***)c;
    for (int i = 0; i < originalCount; i++)
        if (originals[i].table == t)
            return originals[i];
    return originals[0];
}
// Draw recording (deaths): until `recordUntil` (ms), every draw that is not terrain
// goes to Extract\death\draws.txt with its vertex buffer, counts, sprite slot and
// world matrix, so how the game draws a dying monster can be replayed exactly.
static std::atomic<ULONGLONG> recordUntil{0};
static FILE *drawLog = nullptr;
static std::unordered_map<ID3D11Resource *, std::vector<unsigned char>> bufferData; // dynamic VBs written while recording
static std::unordered_map<ID3D11Resource *, uint64_t> bufferVersion, bufferLogged;
static void sample(ID3D11DeviceContext *c, UINT count = 0, UINT start = 0, INT base = 0, bool indexed = false,
                   UINT instances = 0) {
    drawsSeen++;
    ID3D11Buffer *vb = nullptr, *cb = nullptr;
    UINT stride = 0, offset = 0;
    c->IAGetVertexBuffers(0, 1, &vb, &stride, &offset);
    c->VSGetConstantBuffers(2, 1, &cb);
    if (vb && GetTickCount64() < recordUntil) {
        std::lock_guard lock(dataMutex);
        if (!chunks.count(vb) || GetTickCount64() + 2950 < recordUntil) { // the first 50 ms: everything
            if (!drawLog)
                drawLog = _fsopen("Extract\\death\\draws.txt", "a", _SH_DENYNO);
            if (drawLog) {
                auto s = spriteOf.find(vb);
                auto m = cb ? matrices.find(cb) : matrices.end();
                D3D11_BUFFER_DESC desc{};
                vb->GetDesc(&desc);
                ID3D11Buffer *ib = nullptr;
                UINT istride = 0, ioffset = 0;
                c->IAGetVertexBuffers(1, 1, &ib, &istride, &ioffset);
                fprintf(drawLog, "t=%llu vb=%p bytes=%u usage=%d stride=%u %s count=%u start=%u base=%d slot=%d m=",
                        (unsigned long long)GetTickCount64(), (void *)vb, desc.ByteWidth, (int)desc.Usage, stride,
                        indexed ? "indexed" : "draw", count, start, base, s == spriteOf.end() ? -1 : s->second);
                fprintf(drawLog, " instances=%u ivb=%p istride=%u chunk=%d ", instances, (void *)ib, istride, (int)chunks.count(vb));
                if (m != matrices.end())
                    for (float v : m->second)
                        fprintf(drawLog, "%.4f,", v);
                fprintf(drawLog, "\n");
                // The data of each dynamic vertex buffer it reads (once per write).
                for (ID3D11Buffer *b : {vb, ib}) {
                    auto d = b ? bufferData.find(b) : bufferData.end();
                    if (d != bufferData.end() && bufferLogged[b] != bufferVersion[b]) {
                        bufferLogged[b] = bufferVersion[b];
                        fprintf(drawLog, "data vb=%p bytes=%zu floats=", (void *)b, d->second.size());
                        const float *f = (const float *)d->second.data();
                        for (size_t i = 0; i < d->second.size() / 4; i++)
                            fprintf(drawLog, "%g,", f[i]);
                        fprintf(drawLog, "\n");
                    }
                }
                if (ib)
                    ib->Release();
            }
        }
    }
    if (vb && cb) {
        std::lock_guard lock(dataMutex);
        auto m = matrices.find(cb);
        if (m != matrices.end()) {
            drawsWithMatrix++;
            const float *w = m->second.data();
            float length = std::sqrt(w[0] * w[0] + w[1] * w[1] + w[2] * w[2]);
            auto s = spriteOf.find(vb);
            if (s != spriteOf.end()) {
                spriteDraws++;
                auto &list = samples[s->second];
                if (list.size() < 256)
                    list.push_back(length);
            } else if (chunks.count(vb)) {
                chunkDraws++;
                if (chunkSamples.size() < 1024)
                    chunkSamples.push_back(length);
            }
        }
    }
    if (vb)
        vb->Release();
    if (cb)
        cb->Release();
}
static HRESULT STDMETHODCALLTYPE mapHook(ID3D11DeviceContext *c, ID3D11Resource *r, UINT sub, D3D11_MAP type,
                                         UINT flags, D3D11_MAPPED_SUBRESOURCE *out) {
    HRESULT hr = of(c).map(c, r, sub, type, flags, out);
    if (SUCCEEDED(hr) && out && smallConstantBuffer(r)) {
        std::lock_guard lock(dataMutex);
        mapped[r] = out->pData;
    } else if (SUCCEEDED(hr) && out && GetTickCount64() < recordUntil) {
        D3D11_RESOURCE_DIMENSION d{};
        r->GetType(&d);
        D3D11_BUFFER_DESC desc{};
        if (d == D3D11_RESOURCE_DIMENSION_BUFFER)
            static_cast<ID3D11Buffer *>(r)->GetDesc(&desc);
        if (desc.BindFlags & D3D11_BIND_VERTEX_BUFFER && desc.ByteWidth <= (1u << 20)) {
            std::lock_guard lock(dataMutex);
            mapped[r] = out->pData;
        }
    }
    return hr;
}
static void STDMETHODCALLTYPE unmapHook(ID3D11DeviceContext *c, ID3D11Resource *r, UINT sub) {
    {
        std::lock_guard lock(dataMutex);
        auto it = mapped.find(r);
        if (it != mapped.end() && smallConstantBuffer(r)) {
            std::array<float, 16> m;
            memcpy(m.data(), it->second, 64);
            matrices[r] = m;
            mapped.erase(it);
        } else if (it != mapped.end()) {
            D3D11_BUFFER_DESC desc{};
            static_cast<ID3D11Buffer *>(r)->GetDesc(&desc);
            auto &data = bufferData[r];
            data.assign((unsigned char *)it->second, (unsigned char *)it->second + desc.ByteWidth);
            bufferVersion[r]++;
            mapped.erase(it);
        }
    }
    of(c).unmap(c, r, sub);
}
static void STDMETHODCALLTYPE updateHook(ID3D11DeviceContext *c, ID3D11Resource *r, UINT sub, const D3D11_BOX *box,
                                         const void *data, UINT pitch, UINT depth) {
    if (data && !box && smallConstantBuffer(r)) {
        std::array<float, 16> m;
        memcpy(m.data(), data, 64);
        std::lock_guard lock(dataMutex);
        matrices[r] = m;
    }
    of(c).update(c, r, sub, box, data, pitch, depth);
}
static void STDMETHODCALLTYPE draw(ID3D11DeviceContext *c, UINT count, UINT start) {
    sample(c, count, start);
    of(c).draw(c, count, start);
}
static void STDMETHODCALLTYPE drawIndexed(ID3D11DeviceContext *c, UINT count, UINT start, INT base) {
    sample(c, count, start, base, true);
    of(c).drawIndexed(c, count, start, base);
}
static void STDMETHODCALLTYPE drawIndexedInstanced(ID3D11DeviceContext *c, UINT count, UINT instances, UINT start, INT base,
                                                   UINT firstInstance) {
    sample(c, count, start, base, true, instances);
    of(c).drawIndexedInstanced(c, count, instances, start, base, firstInstance);
}
static void STDMETHODCALLTYPE drawInstanced(ID3D11DeviceContext *c, UINT count, UINT instances, UINT start, UINT firstInstance) {
    sample(c, count, start, 0, false, instances);
    of(c).drawInstanced(c, count, instances, start, firstInstance);
}static bool patch(void **slot, void *fn) {
    DWORD old;
    if (!VirtualProtect(slot, sizeof(void *), PAGE_EXECUTE_READWRITE, &old))
        return false;
    *slot = fn;
    VirtualProtect(slot, sizeof(void *), old, &old);
    return true;
}
// Hooks every context the game draws with: the UI graphics' immediate context
// plus the contexts recorded in a terrain chunk's and a sprite's vertex buffer.
static void hookContext(ID3D11DeviceContext *c) {
    if (!c || originalCount >= 8)
        return;
    void **v = *(void ***)c;
    for (int i = 0; i < originalCount; i++)
        if (originals[i].table == v)
            return;
    Originals &o = originals[originalCount];
    o = {v, (DrawIndexed)v[SLOT_DRAW_INDEXED], (DrawFn)v[13], (MapFn)v[SLOT_MAP], (UnmapFn)v[SLOT_UNMAP],
         (UpdateFn)v[SLOT_UPDATE],
         (DrawIndexedInstancedFn)v[SLOT_DRAW_INDEXED_INSTANCED], (DrawInstancedFn)v[SLOT_DRAW_INSTANCED]};
    originalCount++;
    patch(v + SLOT_MAP, (void *)mapHook);
    patch(v + SLOT_UNMAP, (void *)unmapHook);
    patch(v + SLOT_UPDATE, (void *)updateHook);
    patch(v + 13, (void *)draw);
    patch(v + SLOT_DRAW_INDEXED, (void *)drawIndexed);
    patch(v + SLOT_DRAW_INDEXED_INSTANCED, (void *)drawIndexedInstanced);
    patch(v + SLOT_DRAW_INSTANCED, (void *)drawInstanced);
    log("Calibration hooks attached to a context vtable (" + std::to_string(originalCount) + " total)");
}
// Another mod may write its own pointers into these vtable slots after us.
// Re-attach on top of whatever is there now (it becomes the original).
static void keepHooked() {
    for (int i = 0; i < originalCount; i++) {
        Originals &o = originals[i];
        void **v = o.table;
        struct Slot {
            UINT index;
            void *ours;
            void **original;
        } slots[] = {{SLOT_MAP, (void *)mapHook, (void **)&o.map},
                     {SLOT_UNMAP, (void *)unmapHook, (void **)&o.unmap},
                     {SLOT_UPDATE, (void *)updateHook, (void **)&o.update},
                     {13, (void *)draw, (void **)&o.draw},
                     {SLOT_DRAW_INDEXED, (void *)drawIndexed, (void **)&o.drawIndexed},
                     {SLOT_DRAW_INDEXED_INSTANCED, (void *)drawIndexedInstanced, (void **)&o.drawIndexedInstanced},
                     {SLOT_DRAW_INSTANCED, (void *)drawInstanced, (void **)&o.drawInstanced}};
        for (auto &s : slots)
            if (v[s.index] != s.ours) {
                *s.original = v[s.index];
                patch(v + s.index, s.ours);
                static int reports = 0;
                if (reports++ < 20)
                    log("Calibration: vtable slot " + std::to_string(s.index) + " was replaced by another hook; re-attached");
            }
    }
}
static void install(cube::World *w) {
    keepHooked();
    // The game's main context (a global, as tools/MenuProbe.cpp finds it): creatures,
    // sprites and effects are drawn there.
    if (auto main = *(ID3D11DeviceContext **)(base() + 0x551a98))
        hookContext(main);
    if (w->spritemanager && w->spritemanager->gfx_D3D11Graphics)
        hookContext(w->spritemanager->gfx_D3D11Graphics->pImmediateContext);
    if (w->spritemanager)
        for (auto s : w->spritemanager->sprites)
            if (s && s->gfx_D3D11VertexBuffer) {
                hookContext(s->gfx_D3D11VertexBuffer->pImmediateContext);
                break;
            }
    if (TryEnterCriticalSection(&w->zones_mesh_critical_section)) {
        for (auto &[key, zone] : w->zones)
            if (zone)
                for (auto cb : zone->chunk.chunkbuffers)
                    if (cb && cb->vertex_buffer) {
                        hookContext(((gfx::D3D11VertexBuffer *)cb->vertex_buffer)->pImmediateContext);
                        goto done;
                    }
    done:
        LeaveCriticalSection(&w->zones_mesh_critical_section);
    }
}
// Refreshes the buffer -> sprite and chunk tables (game thread).
static void refresh(cube::World *w) {
    std::unordered_map<ID3D11Buffer *, int> sprites;
    if (w->spritemanager)
        for (size_t i = 0; i < w->spritemanager->sprites.size(); i++) {
            auto s = w->spritemanager->sprites[i];
            if (s && s->gfx_D3D11VertexBuffer && s->gfx_D3D11VertexBuffer->buffer)
                sprites[s->gfx_D3D11VertexBuffer->buffer] = (int)i;
        }
    std::unordered_set<ID3D11Buffer *> terrain;
    if (TryEnterCriticalSection(&w->zones_mesh_critical_section)) {
        for (auto &[key, zone] : w->zones)
            if (zone)
                for (auto cb : zone->chunk.chunkbuffers)
                    if (cb && cb->vertex_buffer && ((gfx::D3D11VertexBuffer *)cb->vertex_buffer)->buffer)
                        terrain.insert(((gfx::D3D11VertexBuffer *)cb->vertex_buffer)->buffer);
        LeaveCriticalSection(&w->zones_mesh_critical_section);
    }
    std::lock_guard lock(dataMutex);
    spriteOf.swap(sprites);
    if (!terrain.empty())
        chunks.swap(terrain);
}
static float median(std::vector<float> v) {
    if (v.empty())
        return 0;
    std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
    return v[v.size() / 2];
}
// Measurements from earlier sessions (slot -> line of sprite-scales.txt),
// kept when the file is rewritten and counted as measured by the tour.
static std::map<int, std::string> previous;
static bool previousLoaded = false;
static void loadPrevious() { // caller holds dataMutex
    if (previousLoaded)
        return;
    previousLoaded = true;
    FILE *f = nullptr;
    if (fopen_s(&f, "Extract\\sprite-scales.txt", "r") != 0 || !f)
        return;
    char line[512];
    while (fgets(line, sizeof line, f))
        if (line[0] != '#' && line[0] != '\n')
            previous[atoi(line)] = line;
    fclose(f);
}
// Writes Extract\sprite-scales.txt: slot, blocks per voxel, samples.
static void write(const std::vector<std::pair<void *, std::string>> &names, cube::World *w) {
    std::lock_guard lock(dataMutex);
    loadPrevious();
    float unit = median(chunkSamples);
    log("Calibration: draws " + std::to_string(drawsSeen.load()) + ", with b2 matrix " +
        std::to_string(drawsWithMatrix.load()) + ", sprite draws " + std::to_string(spriteDraws.load()) +
        ", chunk draws " + std::to_string(chunkDraws.load()) + ", sprite buffers " + std::to_string(spriteOf.size()) +
        ", chunk buffers " + std::to_string(chunks.size()) + ", cached matrices " + std::to_string(matrices.size()));
    if (unit <= 0)
        return; // No terrain reference yet; keep the existing file.
    FILE *f = nullptr;
    if (fopen_s(&f, "Extract\\sprite-scales.txt", "w") != 0 || !f)
        return;
    fprintf(f, "# slot\tname\tblocks-per-voxel\tsamples (terrain unit %g from %zu chunk draws)\n", unit,
            chunkSamples.size());
    for (auto &[slot, list] : samples) {
        std::string name = "?";
        if (w->spritemanager && slot < (int)w->spritemanager->sprites.size())
            for (auto it = names.rbegin(); it != names.rend(); ++it)
                if (it->first == w->spritemanager->sprites[slot]) {
                    name = it->second;
                    break;
                }
        fprintf(f, "%d\t%s\t%.5f\t%zu\n", slot, name.c_str(), median(list) / unit, list.size());
    }
    for (auto &[slot, line] : previous)
        if (!samples.count(slot))
            fputs(line.c_str(), f);
    fclose(f);
}
// True once a sprite slot has been measured, in this session or an earlier one.
static bool measured(int slot) {
    std::lock_guard lock(dataMutex);
    loadPrevious();
    auto it = samples.find(slot);
    return (it != samples.end() && !it->second.empty()) || previous.count(slot);
}
} // namespace calibration

// --- Placement records ------------------------------------------------------
// Six std::vectors in cube::Zone hold sprites, objects and spawns (layouts as
// TerrainVariance found them). They are saved raw for decoding.
struct Placement {
    size_t array, stride;
};
// 0xa8: the zone's ambient sound emitters (docs/specs/sound.md §4), read by CubeVulkan's
// ExtractedWorld::soundEmitters.
constexpr Placement PLACEMENTS[] = {{0x18, 0x40}, {0x30, 0xe0}, {0x48, 0xe0},
                                    {0x60, 0xe0}, {0x78, 0xe0}, {0x90, 0x40}, {0xa8, 0x40}};

static std::vector<unsigned char> serialiseRecords(cube::Zone *zone) {
    std::vector<unsigned char> out;
    out.insert(out.end(), {'C', 'W', 'R', '1'});
    for (auto p : PLACEMENTS) {
        unsigned char *range[3];
        memcpy(range, (unsigned char *)zone + p.array, sizeof(range));
        size_t bytes = range[0] && range[1] >= range[0] && range[2] >= range[1] ? (size_t)(range[1] - range[0]) : 0;
        if (bytes % p.stride || bytes > 16 * 1024 * 1024)
            bytes = 0;
        uint32_t stride = (uint32_t)p.stride, count = (uint32_t)(bytes / p.stride), offset = (uint32_t)p.array;
        out.insert(out.end(), (unsigned char *)&offset, (unsigned char *)&offset + 4);
        out.insert(out.end(), (unsigned char *)&stride, (unsigned char *)&stride + 4);
        out.insert(out.end(), (unsigned char *)&count, (unsigned char *)&count + 4);
        if (bytes)
            out.insert(out.end(), range[0], range[0] + bytes);
    }
    return out;
}

static cube::World *session = nullptr;
static int sessionSeed = 0;
static std::string folder;
static std::set<std::pair<int, int>> saved;
static LongVector3 home;

template <class T> static void put(std::vector<unsigned char> &out, T v) {
    auto p = (unsigned char *)&v;
    out.insert(out.end(), p, p + sizeof(T));
}

static int regionType(cube::World *w, int64_t x, int64_t y) {
    IntVector2 region{};
    ((void (*)(cube::World *, IntVector2 *, int, int))(base() + REGION_AT_RVA))(w, &region, (int)x, (int)y);
    return ((int (*)(cube::World *, int, int))(base() + BIOME_AT_RVA))(w, region.x, region.y);
}

// Serialises a fully generated zone (caller holds the zone locks).
static std::vector<unsigned char> serialise(cube::World *w, cube::Zone *zone) {
    std::vector<unsigned char> out;
    out.reserve(256 * 1024);
    out.insert(out.end(), {'C', 'W', 'Z', '1'});
    put<int32_t>(out, zone->position.x);
    put<int32_t>(out, zone->position.y);
    put<int32_t>(out, regionType(w, (int64_t)zone->position.x * 64 + 32, (int64_t)zone->position.y * 64 + 32));
    put<int32_t>(out, 0);
    for (int i = 0; i < 4096; i++) {
        const cube::Field &field = zone->fields[i];
        put<int32_t>(out, field.base_z);
        size_t runsAt = out.size();
        put<uint32_t>(out, 0);
        uint32_t runs = 0;
        const auto &blocks = field.blocks;
        for (size_t j = 0; j < blocks.size();) {
            size_t k = j + 1;
            while (k < blocks.size() && k - j < 65535 && memcmp(&blocks[k], &blocks[j], sizeof(cube::Block)) == 0)
                k++;
            put<uint16_t>(out, (uint16_t)(k - j));
            auto p = (const unsigned char *)&blocks[j];
            out.insert(out.end(), p, p + sizeof(cube::Block));
            runs++;
            j = k;
        }
        memcpy(&out[runsAt], &runs, sizeof(runs));
    }
    return out;
}

// Saves every newly generated zone. Zones with chunk meshes are complete.
static void saveZones(cube::World *w) {
    if (!TryEnterCriticalSection(&w->zones_critical_section))
        return;
    if (!TryEnterCriticalSection(&w->zones_mesh_critical_section)) {
        LeaveCriticalSection(&w->zones_critical_section);
        return;
    }
    int count = 0;
    for (auto &[key, zone] : w->zones) {
        if (!zone || count >= 4)
            continue; // At most four per tick keeps frames smooth.
        std::pair<int, int> id{zone->position.x, zone->position.y};
        if (saved.count(id) || zone->chunk.chunkbuffers.empty())
            continue;
        std::string stem = folder + "\\z_" + std::to_string(id.first) + "_" + std::to_string(id.second);
        Job blocks{stem + ".cwz", serialise(w, zone)}, records{stem + ".rec", serialiseRecords(zone)};
        saved.insert(id);
        count++;
        {
            std::lock_guard lock(jobsMutex);
            jobs.push_back(std::move(blocks));
            jobs.push_back(std::move(records));
        }
        jobsReady.notify_one();
    }
    LeaveCriticalSection(&w->zones_mesh_critical_section);
    LeaveCriticalSection(&w->zones_critical_section);
}

static void startSession(cube::World *w, cube::Creature *player) {
    session = w;
    sessionSeed = w->seed;
    home = player->entity_data.position;
    saved.clear();
    CreateDirectoryA("Extract", nullptr);
    folder = "Extract\\seed_" + std::to_string(w->seed);
    CreateDirectoryA(folder.c_str(), nullptr);
    // Zones saved in an earlier session are kept.
    WIN32_FIND_DATAA data;
    // A zone counts as extracted once its records were saved too (version 2).
    HANDLE h = FindFirstFileA((folder + "\\z_*.rec").c_str(), &data);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            int x, y;
            if (sscanf_s(data.cFileName, "z_%d_%d.rec", &x, &y) == 2)
                saved.insert({x, y});
        } while (FindNextFileA(h, &data));
        FindClose(h);
    }
    std::string manifest = folder + "\\manifest.txt";
    FILE *f = nullptr;
    if (fopen_s(&f, manifest.c_str(), "r") != 0) { // Keep the first home.
        if (fopen_s(&f, manifest.c_str(), "w") == 0 && f) {
            fprintf(f, "seed=%d\nhomeX=%lld\nhomeY=%lld\nhomeZ=%lld\n", w->seed,
                    (long long)(home.x >> 16), (long long)(home.y >> 16), (long long)(home.z >> 16));
            fclose(f);
        }
    } else if (f)
        fclose(f);
    log("Session: seed " + std::to_string(w->seed) + ", " + std::to_string(saved.size()) +
        " zones already extracted, folder " + folder);
}

// Sweep state.
static bool sweeping = false;
static int stop = 0;
static ULONGLONG stopStarted = 0;
static std::vector<std::pair<int, int>> stops; // Zone offsets from home.

static void buildStops() {
    stops.clear();
    // Spiral order from the centre outwards.
    for (int ring = 0; ring <= SWEEP_RADIUS; ring++)
        for (int j = -ring; j <= ring; j++)
            for (int i = -ring; i <= ring; i++)
                if (std::max(std::abs(i), std::abs(j)) == ring)
                    stops.push_back({i * SWEEP_STEP, j * SWEEP_STEP});
}

static void hold(cube::World *w, cube::Creature *player, int64_t bx, int64_t by) {
    float ground = ((float (*)(cube::World *, int, int, int))(base() + GROUND_HEIGHT_RVA))(w, (int)bx, (int)by, 0);
    if (!std::isfinite(ground))
        ground = 0;
    player->entity_data.position.x = bx << 16;
    player->entity_data.position.y = by << 16;
    player->entity_data.position.z = (int64_t)((std::max(ground, 0.f) + 2) * 65536);
    player->entity_data.velocity = FloatVector3(0, 0, 0);
    player->entity_data.HP = ((float (*)(cube::Creature *))(base() + MAX_HP_RVA))(player);
}

// --- Calibration tour ---------------------------------------------------------
// F6: visit the nearest placed instance of every building-part type whose
// size has not been measured yet (from the extracted .rec files), standing on
// each of four sides so the game draws it, until its size is measured.
struct TourStop {
    int slot;
    std::string name;
    int64_t x, y, z; // blocks
};
static std::vector<TourStop> tour;
static size_t tourIndex = 0;
static int tourSide = 0;
static ULONGLONG tourSideStarted = 0;
static bool touring = false;
constexpr ULONGLONG TOUR_SIDE_MS = 4000;

static void buildTour(cube::World *w) {
    tour.clear();
    if (!w->spritemanager)
        return;
    auto manager = (unsigned char *)w->spritemanager;
    void ***parts = (void ***)(manager + 0x50); // building-part sprite vector
    std::unordered_map<void *, int> slotOf;
    for (size_t i = 0; i < w->spritemanager->sprites.size(); i++)
        slotOf[w->spritemanager->sprites[i]] = (int)i;
    std::vector<std::pair<void *, std::string>> names;
    {
        std::lock_guard lock(namesMutex);
        names = spriteNames;
    }
    int64_t hx = home.x >> 16, hy = home.y >> 16;
    std::unordered_map<int, TourStop> best;
    std::unordered_map<int, double> bestDistance;
    WIN32_FIND_DATAA data;
    HANDLE h = FindFirstFileA((folder + "\\z_*.rec").c_str(), &data);
    if (h == INVALID_HANDLE_VALUE)
        return;
    do {
        FILE *f = nullptr;
        if (fopen_s(&f, (folder + "\\" + data.cFileName).c_str(), "rb") != 0 || !f)
            continue;
        std::vector<unsigned char> b;
        unsigned char chunk[65536];
        for (size_t n; (n = fread(chunk, 1, sizeof chunk, f)) > 0;)
            b.insert(b.end(), chunk, chunk + n);
        fclose(f);
        size_t at = 4;
        while (at + 12 <= b.size()) {
            uint32_t offset, stride, count;
            memcpy(&offset, &b[at], 4);
            memcpy(&stride, &b[at + 4], 4);
            memcpy(&count, &b[at + 8], 4);
            at += 12;
            if (!stride || count > (b.size() - at) / stride)
                break;
            size_t position = offset == 0x60 ? 8 : offset == 0x30 ? 0xa0 : 0;
            if (stride == 0xe0 && position)
                for (uint32_t i = 0; i < count; i++) {
                    const unsigned char *r = &b[at + (size_t)i * stride];
                    int32_t id;
                    int64_t p[3];
                    memcpy(&id, r, 4);
                    memcpy(p, r + position, 24);
                    if (id < 0 || parts[0] + id >= parts[1] || (!p[0] && !p[1]))
                        continue;
                    auto slot = slotOf.find(parts[0][id]);
                    if (slot == slotOf.end() || calibration::measured(slot->second))
                        continue;
                    int64_t x = p[0] >> 16, y = p[1] >> 16;
                    double d = std::hypot(double(x - hx), double(y - hy));
                    auto known = bestDistance.find(slot->second);
                    if (known != bestDistance.end() && d >= known->second)
                        continue;
                    std::string name = "?";
                    for (auto &n : names)
                        if (n.first == parts[0][id])
                            name = n.second;
                    best[slot->second] = {slot->second, name, x, y, p[2] >> 16};
                    bestDistance[slot->second] = d;
                }
            at += (size_t)stride * count;
        }
    } while (FindNextFileA(h, &data));
    FindClose(h);
    for (auto &[slot, s] : best)
        tour.push_back(s);
    // Nearest first keeps travel short.
    std::sort(tour.begin(), tour.end(), [&](const TourStop &a, const TourStop &c) {
        return bestDistance[a.slot] < bestDistance[c.slot];
    });
}

static void tourTick(cube::World *w, cube::Creature *player) {
    if (tourIndex >= tour.size()) {
        touring = false;
        player->entity_data.position = home;
        player->entity_data.velocity = FloatVector3(0, 0, 0);
        log("Calibration tour finished; returned home");
        return;
    }
    const TourStop &s = tour[tourIndex];
    static const int sides[4][2] = {{0, -4}, {4, 0}, {0, 4}, {-4, 0}};
    // Stand at the object's own height (indoors and underground objects too).
    player->entity_data.position.x = (s.x + sides[tourSide][0]) << 16;
    player->entity_data.position.y = (s.y + sides[tourSide][1]) << 16;
    player->entity_data.position.z = (s.z + 1) << 16;
    player->entity_data.velocity = FloatVector3(0, 0, 0);
    player->entity_data.HP = ((float (*)(cube::Creature *))(base() + MAX_HP_RVA))(player);
    ULONGLONG now = GetTickCount64();
    bool done = calibration::measured(s.slot);
    if (done || now - tourSideStarted > TOUR_SIDE_MS) {
        if (done || ++tourSide == 4) {
            log("Tour " + std::to_string(tourIndex + 1) + "/" + std::to_string(tour.size()) + ": " + s.name +
                (done ? " measured" : " not drawn from any side"));
            tourIndex++;
            tourSide = 0;
        }
        tourSideStarted = now;
    }
}

// --- Character recording ---------------------------------------------------------
// The game keeps each creature's final pose in Creature::AnimationState: the
// sprite drawn for every body part and one world matrix per part. F5 writes a
// readable snapshot of the player; F4 toggles recording every tick to a binary
// file, so animations can be replayed exactly without the game's code.
// The game's own combat numbers for a creature, from its stat functions
// (RVAs as in CWSDK's Creature.cpp).
static std::string statsText(cube::Creature *c) {
    auto call = [&](uintptr_t rva) { return ((float (*)(cube::Creature *))(base() + rva))(c); };
    auto callBool = [&](uintptr_t rva) { return ((float (*)(cube::Creature *, bool))(base() + rva))(c, true); };
    float critical = ((float (*)(cube::Creature *, cube::Creature *, bool))(base() + 0x50D30))(c, nullptr, true);
    char s[360];
    sprintf_s(s, "hp=%.2f maxhp=%.2f attack=%.3f spell=%.3f armor=%.3f resistance=%.3f critical=%.4f haste=%.4f regen=%.4f managen=%.4f",
              c->entity_data.HP, call(MAX_HP_RVA), callBool(0x4FA70), callBool(0x65C70), call(0x4F080), call(0x64D90),
              critical, call(0x66460), call(0x647B0), call(0x5F8D0));
    return s;
}

// --- Combat log ------------------------------------------------------------------
// F2: fight mode. Puts the player next to the nearest monster and logs every
// health change of the player and of creatures within 30 blocks, with who was
// attacking (ability ids), to Extract\combat-log.txt. The player is refilled
// below 30% health so the fight can go on.
// --- Interface capture ------------------------------------------------------------
// F10 saves the game's next few rendered frames (before ReShade's effects) as
// Extract\ui\frame-<n>.bmp, so CubeVulkan can use the game's own interface art.
// Present is hooked through the swap-chain class's method table, found from a
// throwaway swap chain.
namespace uicapture {
using Present = HRESULT(STDMETHODCALLTYPE *)(IDXGISwapChain *, UINT, UINT);
static Present originalPresent = nullptr;
using Present1 = HRESULT(STDMETHODCALLTYPE *)(IDXGISwapChain *, UINT, UINT, const void *);
static Present1 originalPresent1 = nullptr;
static std::atomic<bool> seen{false};
static std::atomic<int> pending{0};
static ULONGLONG nextShot = 0;
static std::atomic<int> spacing{300}; // ms between saved frames
static std::string folder = "Extract\\ui"; // where they go (deaths: Extract\death)
static int shot = 0;

static void save(IDXGISwapChain *chain) {
    ID3D11Texture2D *back = nullptr;
    if (FAILED(chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&back)) || !back) {
        extract::log("Interface capture: no back buffer");
        return;
    }
    ID3D11Device *device = nullptr;
    back->GetDevice(&device);
    ID3D11DeviceContext *context = nullptr;
    device->GetImmediateContext(&context);
    D3D11_TEXTURE2D_DESC desc;
    back->GetDesc(&desc);
    D3D11_TEXTURE2D_DESC sd = desc;
    sd.Usage = D3D11_USAGE_STAGING;
    sd.BindFlags = 0;
    sd.MiscFlags = 0;
    sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    sd.SampleDesc = {1, 0};
    ID3D11Texture2D *staging = nullptr, *resolved = nullptr;
    if (desc.SampleDesc.Count > 1) { // Multisampled back buffer: resolve it first.
        D3D11_TEXTURE2D_DESC rd = desc;
        rd.SampleDesc = {1, 0};
        rd.BindFlags = 0;
        rd.MiscFlags = 0;
        if (SUCCEEDED(device->CreateTexture2D(&rd, nullptr, &resolved)))
            context->ResolveSubresource(resolved, 0, back, 0, desc.Format);
    }
    if (desc.SampleDesc.Count > 1 && !resolved)
        extract::log("Interface capture: cannot resolve the multisampled back buffer");
    else if (FAILED(device->CreateTexture2D(&sd, nullptr, &staging)))
        extract::log("Interface capture: cannot create a staging texture (format " + std::to_string(desc.Format) + ")");
    else {
        context->CopyResource(staging, resolved ? resolved : back);
        D3D11_MAPPED_SUBRESOURCE m;
        if (SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &m))) {
            bool bgra = desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM || desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
            CreateDirectoryA("Extract", nullptr);
            CreateDirectoryA(folder.c_str(), nullptr);
            // Named by the time they were taken: frames of a death never replace interface captures.
            std::string path = folder + "\\frame-" + std::to_string(GetTickCount64()) + "-" + std::to_string(shot++) + ".bmp";
            FILE *f = nullptr;
            if (fopen_s(&f, path.c_str(), "wb") == 0 && f) {
                // 24-bit bottom-up BMP.
                uint32_t w = desc.Width, h = desc.Height, row = (w * 3 + 3) & ~3u, size = 54 + row * h;
                unsigned char header[54] = {'B', 'M'};
                memcpy(header + 2, &size, 4);
                header[10] = 54;
                header[14] = 40;
                memcpy(header + 18, &w, 4);
                memcpy(header + 22, &h, 4);
                header[26] = 1;
                header[28] = 24;
                fwrite(header, 1, 54, f);
                std::vector<unsigned char> line(row, 0);
                for (uint32_t y = 0; y < h; y++) {
                    const unsigned char *src = (const unsigned char *)m.pData + (size_t)(h - 1 - y) * m.RowPitch;
                    for (uint32_t x = 0; x < w; x++) {
                        const unsigned char *p = src + x * 4;
                        line[x * 3 + 0] = bgra ? p[0] : p[2];
                        line[x * 3 + 1] = p[1];
                        line[x * 3 + 2] = bgra ? p[2] : p[0];
                    }
                    fwrite(line.data(), 1, row, f);
                }
                fclose(f);
                extract::log("Saved " + path + " (" + std::to_string(w) + "x" + std::to_string(h) + ", format " +
                             std::to_string(desc.Format) + ")");
            }
            context->Unmap(staging, 0);
        }
        staging->Release();
    }
    if (resolved)
        resolved->Release();
    context->Release();
    device->Release();
    back->Release();
}

static void maybeSave(IDXGISwapChain *chain) {
    if (!seen.exchange(true))
        extract::log("Interface capture: the game presents through the hooked swap chain");
    if (pending > 0 && GetTickCount64() >= nextShot) {
        save(chain);
        pending--;
        nextShot = GetTickCount64() + spacing;
    }
}
static HRESULT STDMETHODCALLTYPE present1(IDXGISwapChain *chain, UINT sync, UINT flags, const void *params) {
    maybeSave(chain);
    return originalPresent1(chain, sync, flags, params);
}
static HRESULT STDMETHODCALLTYPE present(IDXGISwapChain *chain, UINT sync, UINT flags) {
    maybeSave(chain);
    return originalPresent(chain, sync, flags);
}
static void install() {
    static bool done = false;
    if (done)
        return;
    done = true;
    WNDCLASSA wc{};
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "ZoneExtractorDummy";
    RegisterClassA(&wc);
    HWND window = CreateWindowA(wc.lpszClassName, "", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr,
                                wc.hInstance, nullptr);
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = window;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    IDXGISwapChain *chain = nullptr;
    ID3D11Device *device = nullptr;
    ID3D11DeviceContext *context = nullptr;
    if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
                                                D3D11_SDK_VERSION, &sd, &chain, &device, nullptr, &context))) {
        void **table = *(void ***)chain;
        DWORD old;
        if (VirtualProtect(&table[8], sizeof(void *), PAGE_EXECUTE_READWRITE, &old)) {
            originalPresent = (Present)table[8];
            table[8] = (void *)&present;
            VirtualProtect(&table[8], sizeof(void *), old, &old);
            IDXGISwapChain1 *chain1 = nullptr;
            if (SUCCEEDED(chain->QueryInterface(__uuidof(IDXGISwapChain1), (void **)&chain1)) && chain1) {
                void **t1 = *(void ***)chain1;
                if (VirtualProtect(&t1[22], sizeof(void *), PAGE_EXECUTE_READWRITE, &old)) {
                    originalPresent1 = (Present1)t1[22];
                    t1[22] = (void *)&present1;
                    VirtualProtect(&t1[22], sizeof(void *), old, &old);
                }
                chain1->Release();
            }
            extract::log("Interface capture ready (F10)");
        }
        chain->Release();
        device->Release();
        context->Release();
    } else
        extract::log("Interface capture unavailable: no dummy swap chain");
    DestroyWindow(window);
}
} // namespace uicapture

// Each character's recordings go to their own folder, Extract\characters\<name>-<class>
// (snapshot, animations, gear survey), so several characters (one of each class)
// can be recorded without overwriting each other. Created on first use.
static const char *className(unsigned c) {
    switch (c) {
    case 1: return "warrior";
    case 2: return "ranger";
    case 3: return "mage";
    case 4: return "rogue";
    default: return "class";
    }
}
// Each class has two specializations (the game's subclasses), with their own skills.
static std::string specializationName(cube::Creature *c) {
    static const char *names[5][2] = {
        {"class", "class"}, {"berserker", "guardian"}, {"sniper", "scout"}, {"fire", "water"}, {"assassin", "ninja"}};
    unsigned k = c->entity_data.classType;
    int s = c->entity_data.specialization;
    if (k > 4 || s < 0 || s > 1)
        return std::string(className(k)) + "-" + std::to_string(s);
    return names[k][s];
}
// F7: switches the player to the class's other specialization, as a town's guild
// receptionist does, so its skills can be recorded with the game's own
// animations. F7 again switches back (do so before leaving: the game saves it).
static void switchSpecialization(cube::Creature *c) {
    char &s = c->entity_data.specialization;
    s = s == 0 ? 1 : 0;
    extract::log("Specialization switched to " + specializationName(c) + " (F7 again switches back)");
}
// X (unused by the game): fills the player's MP (the game keeps it as 0..1), so skills that spend
// MP, which the game only grants for hits on enemies, can be recorded without
// a fight. MP is not kept between sessions.
static void fillMana(cube::Creature *c) {
    c->entity_data.MP = 1.f;
    extract::log("MP filled for recording");
}
static std::string characterFolder(cube::Creature *c) {
    std::string name;
    for (char ch : std::string(c->entity_data.name, strnlen(c->entity_data.name, sizeof c->entity_data.name)))
        name += isalnum((unsigned char)ch) || ch == '-' || ch == '_' ? (char)tolower((unsigned char)ch) : '_';
    if (name.empty())
        name = "unnamed";
    std::string folder = "Extract\\characters\\" + name + "-" + specializationName(c);
    CreateDirectoryA("Extract", nullptr);
    CreateDirectoryA("Extract\\characters", nullptr);
    CreateDirectoryA(folder.c_str(), nullptr);
    return folder;
}

// --- Gear survey --------------------------------------------------------------------
// F8: records every item the player carries, as the game sees it. One slot at a
// time: the slot is emptied (the player's base stats), then each fitting item
// is equipped for a few ticks while the game rebuilds the character, and the
// sprite it draws, the player's stats with it and the item's own armor, HP and
// price are written to Extractaracters<name>-<class>gear.txt. The original equipment is
// copied back byte for byte at the end (and if the survey is stopped).
namespace gear {
constexpr int SETTLE_TICKS = 20, TEST_TICKS = 8, MAX_TICKS = 240; // the game rebuilds the character after a gear change
constexpr uintptr_t ITEM_ARMOR = 0x108D50, ITEM_HP = 0x1097B0, ITEM_PRICE = 0x109D30;
static const char *SLOT_NAMES[] = {"unk", "neck", "chest", "feet", "hands", "shoulder", "weapon_left",
                                   "weapon_right", "ring_left", "ring_right", "pet"};
struct Step {
    int slot;         // equipment slot (Equipment order above)
    int tab = -1, index = -1; // inventory position; -1 = empty the slot (baseline), -2 = original, -3 = test item
    int category = 0, id = 0, material = 0; // test item
};
// Catalogue: test items of every equippable category and id. Materials tried
// in turn until the game draws a model for the type (a type only exists in
// some materials); then each category's first type in every material.
constexpr int TEST_IDS = 26, TEST_MATERIALS = 41;
static const int TRY_MATERIALS[] = {1, 2, 11, 12, 25, 26, 27, 28, 29, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15};
static int catalogueCategory = 0, catalogueId = 0, catalogueTry = 0;
static bool active = false;
static unsigned char original[sizeof(cube::Equipment)];
static std::vector<Step> steps;
static size_t step = 0;
static int wait = 0, waited = 0;
static bool fresh = true; // the current step has not been applied yet
static std::set<std::pair<int, int>> found; // (category, id) the game drew a model for
static std::map<int, int> firstId;           // category -> first id with a model
static FILE *out = nullptr;

static cube::Item *slotItem(cube::Creature *c, int slot) { return &c->entity_data.equipment.unk_item + slot; }

// Equipment slot for an item category, as the game uses them (checked by the
// sprite each one makes the game draw; logged either way).
static int slotFor(int category) {
    switch (category) {
    case 3: return 7; // weapon (right hand)
    case 4: return 2; // chest
    case 5: return 4; // hands
    case 6: return 3; // feet
    case 7: return 5; // shoulder
    case 8: return 1; // amulet (worn at the neck, confirmed on creatures)
    case 9: return 8; // ring
    default: return -1;
    }
}

static void itemText(FILE *f, const cube::Item &it) {
    fprintf(f, "category=%d id=%d material=%d rarity=%d modifier=%u region=%d,%d formula=%d spirits=%d",
            it.category, it.id, it.material, it.rarity, it.modifier, it.region.x, it.region.y, it.formula_category,
            it.num_spirits);
}

static void begin(cube::World *w) {
    cube::Creature *p = w->local_creature;
    memcpy(original, &p->entity_data.equipment, sizeof original);
    steps.clear();
    for (int slot : {2, 3, 4, 5, 7, 1, 8}) {
        steps.push_back({slot});
        for (int t = 0; t < (int)p->inventory_tabs.size(); t++)
            for (int i = 0; i < (int)p->inventory_tabs[t].size(); i++) {
                const cube::Item &it = p->inventory_tabs[t][i].item;
                if (slotFor(it.category) == slot)
                    steps.push_back({slot, t, i});
            }
        // What was equipped there originally.
        steps.push_back({slot, -2, -2});
    }
    // Catalogue of test items: disabled. The game removes equipped items the
    // player does not own before drawing them, so item types are catalogued from
    // the gear creatures wear instead (creature tour, creatures.txt "equip=").
    for (int category = 3; category <= 9 && false; category++) {
        int slot = slotFor(category);
        for (int id = 0; id < TEST_IDS; id++)
            for (int m : TRY_MATERIALS)
                steps.push_back({slot, -3, -3, category, id, m});
        for (int m = 0; m < TEST_MATERIALS; m++)
            steps.push_back({slot, -3, -3, category, -1, m}); // -1: the category's first found id
        steps.push_back({slot, -2, -2});
    }
    fopen_s(&out, (characterFolder(p) + "\\gear.txt").c_str(), "w");
    if (out) {
        fprintf(out, "# Gear survey: player class=%u specialization=%d level=%u\n", p->entity_data.classType,
                p->entity_data.specialization, p->entity_data.level);
        for (int t = 0; t < (int)p->inventory_tabs.size(); t++)
            for (int i = 0; i < (int)p->inventory_tabs[t].size(); i++) {
                const auto &s = p->inventory_tabs[t][i];
                fprintf(out, "inventory tab=%d index=%d quantity=%d ", t, i, s.quantity);
                itemText(out, s.item);
                fprintf(out, " price=%d\n", ((int (*)(const cube::Item *))(base() + ITEM_PRICE))(&s.item));
            }
        for (int s = 0; s < 11; s++) {
            const cube::Item &it = ((const cube::Item *)original)[s];
            fprintf(out, "equipped slot=%s ", SLOT_NAMES[s]);
            itemText(out, it);
            fprintf(out, "\n");
        }
    }
    step = 0;
    wait = SETTLE_TICKS;
    fresh = true;
    found.clear();
    firstId.clear();
    active = true;
    extract::log("Gear survey started: " + std::to_string(steps.size()) + " steps");
}

static void finish(cube::World *w, const char *why) {
    memcpy(&w->local_creature->entity_data.equipment, original, sizeof original);
    if (out)
        fclose(out);
    out = nullptr;
    active = false;
    extract::log(std::string("Gear survey ") + why + "; original equipment restored");
}

static void apply(cube::Creature *p, const Step &s) {
    cube::Item *target = slotItem(p, s.slot);
    if (s.tab == -2)
        memcpy(target, &((const cube::Item *)original)[s.slot], sizeof(cube::Item));
    else if (s.tab < 0)
        memset(target, 0, sizeof(cube::Item)); // empty slot (category 0)
    else if (s.tab == -3) {
        // A test item: only its type and material; region where the player is.
        memset(target, 0, sizeof(cube::Item));
        target->category = (char)s.category;
        target->id = s.id;
        target->material = (char)s.material;
        target->region = p->entity_data.current_region;
    } else
        memcpy(target, &p->inventory_tabs[s.tab][s.index].item, sizeof(cube::Item));
}

static void tick(cube::World *w) {
    if (!active)
        return;
    cube::Creature *p = w->local_creature;
    if (step >= steps.size()) {
        finish(w, "finished");
        return;
    }
    Step s = steps[step];
    if (fresh) {
        fresh = false;
        wait = s.tab == -3 ? TEST_TICKS : SETTLE_TICKS;
        if (s.tab == -3 && s.id >= 0 && found.count({s.category, s.id})) {
            step++; // Already catalogued in another material.
            fresh = true;
            return;
        }
        if (s.tab == -3 && s.id < 0) {
            auto f = firstId.find(s.category);
            if (f == firstId.end()) {
                step++;
                fresh = true;
                return;
            }
            steps[step].id = s.id = f->second;
            steps[step].tab = -4; // material sweep (logged as such)
            s.tab = -3;
        }
        apply(p, s);
        waited = 0;
    }
    waited++;
    if (wait > 1) {
        wait--;
        return;
    }
    // Then until the rebuilt character has its sprites and health again.
    bool rebuilt = p->animation_state.face_sprite && p->entity_data.HP > 0;
    if (!rebuilt && waited < MAX_TICKS)
        return;
    wait = 0;
    // The game has had a few ticks to rebuild the character with this item.
    if (out) {
        const char *kind = s.tab == -1 ? "baseline" : s.tab == -2 ? "restored" : steps[step].tab == -4 ? "material" : s.tab == -3 ? "test" : "item";
        fprintf(out, "%s slot=%s waited=%d ", kind, SLOT_NAMES[s.slot], waited);
        if (s.tab >= 0)
            fprintf(out, "tab=%d index=%d ", s.tab, s.index);
        const cube::Item *it = slotItem(p, s.slot);
        itemText(out, *it);
        auto armor = ((float (*)(const cube::Item *, cube::Creature *))(base() + ITEM_ARMOR))(it, p);
        auto hp = ((float (*)(const cube::Item *, cube::Creature *))(base() + ITEM_HP))(it, p);
        fprintf(out, " item_armor=%.3f item_hp=%.3f | player %s | sprites", armor, hp, statsText(p).c_str());
        int64_t *sprites = &p->animation_state.face_sprite;
        for (int i = 0; i < 12; i++) {
            std::string n;
            if (sprites[i]) {
                std::lock_guard lock(namesMutex);
                for (auto &entry : spriteNames)
                    if (entry.first == (void *)sprites[i])
                        n = entry.second;
            }
            if (n.rfind("sprites/", 0) == 0)
                n = n.substr(8);
            fprintf(out, " %s", n.empty() ? "-" : n.c_str());
        }
        // Weapons: where the game puts them relative to the hands (matrices 7-10:
        // weapon right, weapon left, left hand, right hand).
        if (s.slot == 7 || s.slot == 6) {
            fprintf(out, " | matrices");
            const Matrix4 *m = &p->animation_state.body_with_head_matrix;
            for (int k = 7; k <= 10; k++) {
                const float *f = &m[k]._11;
                for (int j = 0; j < 16; j++)
                    fprintf(out, " %.5f", f[j]);
            }
        }
        fprintf(out, "\n");
        fflush(out);
        // A test item counts as a real type when the game drew a model for it in
        // its slot (rings have none: they count when they add stats).
        if (s.tab == -3 && steps[step].tab == -3) {
            static const int column[] = {-1, 11, 2, 7, 4, 10, 9, 8, -1, -1, -1};
            int c = column[s.slot];
            std::string drawn;
            if (c >= 0 && sprites[c]) {
                std::lock_guard lock(namesMutex);
                for (auto &entry : spriteNames)
                    if (entry.first == (void *)sprites[c])
                        drawn = entry.second;
            }
            bool isDefault = drawn.empty() || drawn.find("body") != std::string::npos ||
                             drawn.find("/hand") != std::string::npos || drawn.find("/foot") != std::string::npos;
            if ((c >= 0 && !isDefault) || (c < 0 && (armor > 0 || hp > 0))) {
                found.insert({s.category, s.id});
                if (!firstId.count(s.category))
                    firstId[s.category] = s.id;
            }
        }
    }
    step++;
    fresh = true;
}

// Class rules: the game's own check (Item*, class) -> may use, found at 0x1094d0
// (compares the category with 3/7, then per weapon id `class == n`). It is a
// pure function of the item, so it is asked directly for every category, id
// and material for classes 1-4; nothing is equipped. If a mod has hooked it
// (a jump at its start), the module it jumps into is logged, since a mod may
// change the rules. Output: Extractass-rules.txt.
constexpr uintptr_t CLASS_CHECK = 0x1094d0;
static void classRules(cube::Creature *p) {
    FILE *f = nullptr;
    CreateDirectoryA("Extract", nullptr);
    if (fopen_s(&f, "Extract\\class-rules.txt", "w") != 0 || !f)
        return;
    const unsigned char *entry = (const unsigned char *)base() + CLASS_CHECK;
    if (entry[0] == 0xff && entry[1] == 0x25) { // jmp [rip+0]: hooked
        uintptr_t target;
        memcpy(&target, entry + 6, 8);
        HMODULE module = nullptr;
        char name[MAX_PATH] = "?";
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR)target, &module))
            GetModuleFileNameA(module, name, MAX_PATH);
        fprintf(f, "# class check hooked by %s\n", name);
        extract::log(std::string("Class check is hooked by ") + name);
    } else
        fprintf(f, "# class check not hooked (vanilla)\n");
    fprintf(f, "# player class=%u; columns: category id material then may-use for classes 1 2 3 4\n",
            p->entity_data.classType);
    using Check = bool (*)(const cube::Item *, int);
    auto check = (Check)(base() + CLASS_CHECK);
    for (int category = 1; category <= 30; category++)
        for (int id = 0; id < 40; id++)
            for (int material = 0; material < 41; material++) {
                alignas(cube::Item) unsigned char raw[sizeof(cube::Item)] = {};
                cube::Item &item = *(cube::Item *)raw; // plain data; the SDK constructor is not linked
                item.category = (char)category;
                item.id = id;
                item.material = (char)material;
                fprintf(f, "%d %d %d %d %d %d %d\n", category, id, material, (int)check(&item, 1),
                        (int)check(&item, 2), (int)check(&item, 3), (int)check(&item, 4));
            }
    fclose(f);
    extract::log("Class rules written to Extract\\class-rules.txt");
}

// Bytes at the start of the item functions CWSDK names, to check their roles.
static void dumpFunctions() {
    static bool done = false;
    if (done)
        return;
    done = true;
    // Every function start (after int3 padding) around the item functions, to
    // find the game's class check among them (Extractitem-functions.txt).
    FILE *f = nullptr;
    if (fopen_s(&f, "Extract\\item-functions.txt", "w") == 0 && f) {
        const unsigned char *code = (const unsigned char *)base();
        for (uintptr_t rva = 0x106000; rva < 0x10C000; rva++)
            if (code[rva - 1] == 0xcc && code[rva] != 0xcc) {
                fprintf(f, "%llx", (unsigned long long)rva);
                for (int i = 0; i < 160; i++)
                    fprintf(f, "%s%02x", i ? "" : " ", code[rva + i]);
                fprintf(f, "\n");
            }
        fclose(f);
    }
    for (uintptr_t rva : {ITEM_ARMOR, ITEM_HP, ITEM_PRICE, (uintptr_t)0x109720}) {
        std::string line = "Item function " + std::to_string(rva) + ":";
        char b[4];
        for (int i = 0; i < 96; i++) {
            sprintf_s(b, "%02x", ((unsigned char *)base() + rva)[i]);
            line += b;
        }
        extract::log(line);
    }
}
} // namespace gear

namespace combat {
static bool active = false;
static int64_t targetId = 0; // the monster fight mode keeps the camera on
static ULONGLONG targetDiedAt = 0; // the camera stays on it while it dies
static std::unordered_map<int64_t, float> lastHp;
static FILE *log = nullptr;

static double distance(cube::Creature *a, cube::Creature *b) {
    double dx = double(a->entity_data.position.x - b->entity_data.position.x) / 65536,
           dy = double(a->entity_data.position.y - b->entity_data.position.y) / 65536,
           dz = double(a->entity_data.position.z - b->entity_data.position.z) / 65536;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

static void toggle(cube::World *w) {
    active = !active;
    cube::Creature *player = w->local_creature;
    if (!active) {
        if (log)
            fclose(log);
        log = nullptr;
        extract::log("Fight mode off");
        return;
    }
    CreateDirectoryA("Extract", nullptr);
    log = _fsopen("Extract\\combat-log.txt", "a", _SH_DENYNO); // readable while the game runs
    lastHp.clear();
    // Nearest monster (hostility 1) the game is simulating.
    cube::Creature *best = nullptr;
    for (cube::Creature *c : w->creatures)
        if (c && c != player && c->entity_data.hostility_type == 1 && c->entity_data.HP > 0 &&
            (!best || distance(c, player) < distance(best, player)))
            best = c;
    if (log)
        fprintf(log, "# fight start; player %s\n", statsText(player).c_str());
    if (best) {
        targetId = best->id;
        player->entity_data.position = best->entity_data.position;
        player->entity_data.position.x += 3 << 16;
        player->entity_data.velocity = FloatVector3(0, 0, 0);
        if (log)
            fprintf(log, "# target id=%lld race=%u level=%u %s\n", (long long)best->id, best->entity_data.race,
                    best->entity_data.level, statsText(best).c_str());
        extract::log("Fight mode on: next to monster race " + std::to_string(best->entity_data.race) + " at " +
                     std::to_string(distance(best, player)) + " blocks");
    } else
        extract::log("Fight mode on: no monster nearby");
}

static void tick(cube::Game *g) {
    cube::World *w = g->world;
    if (!active || !log)
        return;
    cube::Creature *player = w->local_creature;
    auto &pe = player->entity_data;
    // Keep the camera (attacks follow it) on the target, or the nearest living monster.
    {
        cube::Creature *target = nullptr;
        for (cube::Creature *c : w->creatures)
            if (c && c != player && c->entity_data.hostility_type == 1 &&
                (c->entity_data.HP > 0 || (c->id == targetId && GetTickCount64() - targetDiedAt < 7000)) &&
                (c->id == targetId || !target || (target->id != targetId && distance(c, player) < distance(target, player))))
                target = c;
        if (target) {
            targetId = target->id;
            double dx = double(target->entity_data.position.x - pe.position.x), dy = double(target->entity_data.position.y - pe.position.y);
            double yaw = std::atan2(dy, dx) * 180 / 3.14159265358979 +
                         GetPrivateProfileIntA("Fight", "YawOffset", 90, ".\\Mods\\ZoneExtractor.ini");
            g->camera_angle.z = g->target_camera_angle.z = yaw;
            g->camera_angle.x = g->target_camera_angle.x = 75;
        }
    }
    for (cube::Creature *c : w->creatures) {
        if (!c || (c != player && distance(c, player) > 30))
            continue;
        float hp = c->entity_data.HP;
        auto it = lastHp.find(c->id);
        if (it != lastHp.end() && std::abs(it->second - hp) > .001f) {
            // Who was acting: the nearest other creature, and the player.
            cube::Creature *closest = nullptr;
            for (cube::Creature *o : w->creatures)
                if (o && o != c && (!closest || distance(o, c) < distance(closest, c)))
                    closest = o;
            fprintf(log,
                    "t=%llu target=%s id=%lld race=%u level=%u hp=%.2f->%.2f | player ability=%d since=%.0f | "
                    "nearest id=%lld race=%u ability=%d since=%.0f dist=%.2f\n",
                    (unsigned long long)GetTickCount64(), c == player ? "player" : "creature", (long long)c->id,
                    c->entity_data.race, c->entity_data.level, it->second, hp, pe.current_ability,
                    pe.time_since_ability, closest ? (long long)closest->id : -1LL, closest ? closest->entity_data.race : 0u,
                    closest ? closest->entity_data.current_ability : 0, closest ? closest->entity_data.time_since_ability : 0.f,
                    closest ? distance(closest, c) : -1.0);
            fflush(log);
        }
        lastHp[c->id] = hp;
    }
    // A monster that just died: step onto where it fell a second later, so its
    // pouch is picked up and logged below.
    static LongVector3 fell;
    static ULONGLONG fellAt = 0;
    static std::unordered_map<int64_t, bool> alive;
    for (cube::Creature *c : w->creatures) {
        if (!c || c == player || c->entity_data.hostility_type != 1)
            continue;
        bool living = c->entity_data.HP > 0;
        bool was = alive.count(c->id) ? alive[c->id] : living;
        alive[c->id] = living;
        if (was && !living) {
            fell = c->entity_data.position;
            fellAt = GetTickCount64();
            if (c->id == targetId)
                targetDiedAt = fellAt;
            fprintf(log, "t=%llu died id=%lld race=%u stars=%u\n", (unsigned long long)fellAt, (long long)c->id,
                    c->entity_data.race, c->entity_data.level);
        }
    }
    if (fellAt && GetTickCount64() - fellAt > 7000) {
        pe.position = fell;
        pe.velocity = FloatVector3(0, 0, 0);
        fellAt = 0;
    }
    // MP, stamina and charge: how hits fill them and skills spend them.
    static float lastMp = -1, lastStamina = -1;
    static int lastCharge = -1;
    if (std::abs(pe.MP - lastMp) > .005f || std::abs(player->stamina - lastStamina) > .005f || pe.charge != lastCharge) {
        fprintf(log, "t=%llu resources mp=%.4f stamina=%.4f charge=%d ability=%d since=%.0f\n",
                (unsigned long long)GetTickCount64(), pe.MP, player->stamina, (int)pe.charge, pe.current_ability,
                pe.time_since_ability);
        lastMp = pe.MP, lastStamina = player->stamina, lastCharge = pe.charge;
    }
    // Loot: what the player gains (picking up a monster's pouch), item by item, and gold.
    static std::map<std::string, int> carried;
    static int lastGold = INT_MIN;
    std::map<std::string, int> now;
    for (auto &tab : player->inventory_tabs)
        for (auto &stack : tab) {
            const cube::Item &it = stack.item;
            char key[128];
            snprintf(key, sizeof key, "category=%d id=%d material=%d rarity=%d modifier=%u level-region=%d,%d",
                     (int)it.category, it.id, (int)it.material, (int)it.rarity, it.modifier, it.region.x, it.region.y);
            now[key] += stack.quantity;
        }
    if (lastGold != INT_MIN) {
        for (auto &[key, count] : now) {
            int before = carried.count(key) ? carried[key] : 0;
            if (count > before)
                fprintf(log, "t=%llu loot +%d %s\n", (unsigned long long)GetTickCount64(), count - before, key.c_str());
        }
        if (player->gold != lastGold)
            fprintf(log, "t=%llu gold %d->%d\n", (unsigned long long)GetTickCount64(), lastGold, player->gold);
    }
    carried = now;
    lastGold = player->gold;
    float maxHp = ((float (*)(cube::Creature *))(base() + MAX_HP_RVA))(player);
    if (pe.HP < maxHp * .3f) {
        pe.HP = maxHp;
        lastHp[player->id] = maxHp;
        fprintf(log, "# player refilled\n");
    }
}
} // namespace combat

// --- Hit probe ---------------------------------------------------------------------
// The game applies every hit through one function (RVA 0x29eb60): the world, the
// target, the attacker, the damage before the target's defense, then whether it
// was critical, ..., the hit type (1 or 2: magic, defended by resistance) and the
// ability. Every call is logged to Extract\hits.txt with the attacker's and
// target's stats from the game's own functions, and the health actually taken,
// so CubeVulkan's damage rules can be checked against the game. Read-only: the
// original runs unchanged.
namespace hitprobe {
constexpr uintptr_t APPLY_HIT_RVA = 0x29eb60, MAX_COMBO_RVA = 0x5f9e0;
using ApplyHit = char (*)(void *, cube::Creature *, cube::Creature *, float, uint8_t, uint8_t, float, void *, void *,
                          int, int, int64_t, uint8_t);
static ApplyHit original = nullptr;
static FILE *log = nullptr;
// While the update probe records a call on this thread: each hit it applies (ids, damage,
// critical, flag, charge, the place and push given, type, ability, extra, last).
static thread_local std::vector<uint8_t> *calls = nullptr;
// The hit oracle's recorder (hitoracle::record), which calls the original itself.
static ApplyHit recorder = nullptr;
// While the effect oracle records a step on this thread: each hit's result (one byte).
static thread_local std::vector<uint8_t> *results = nullptr;
// While the update probe records a call: the rand() values drawn so far (its list), and
// for each hit how many had been drawn before and after it, and its result.
static thread_local std::vector<int32_t> *drawsSeen = nullptr;
static thread_local std::vector<uint8_t> *spans = nullptr;

static char hook(void *world, cube::Creature *target, cube::Creature *attacker, float damage, uint8_t critical,
                 uint8_t flag, float charge, void *where, void *push, int type, int ability, int64_t extra,
                 uint8_t last) {
    if (calls) {
        auto put = [&](const void *p, size_t n) { calls->insert(calls->end(), (const uint8_t *)p, (const uint8_t *)p + n); };
        const int64_t none = 0, tid = target ? target->id : 0, aid = attacker ? attacker->id : 0;
        put(&tid, 8), put(&aid, 8), put(&damage, 4), put(&critical, 1), put(&flag, 1), put(&charge, 4);
        int64_t at[3] = {};
        float push3[3] = {};
        if (where)
            memcpy(at, where, sizeof at);
        if (push)
            memcpy(push3, push, sizeof push3);
        put(at, sizeof at), put(push3, sizeof push3), put(&type, 4), put(&ability, 4), put(&extra, 8), put(&last, 1);
        (void)none;
    }
    float before = target ? target->entity_data.HP : 0;
    char text[512] = "";
    if (target && attacker) {
        auto *a = (uint8_t *)attacker;
        auto stat = [&](uintptr_t rva, cube::Creature *c) { return ((float (*)(cube::Creature *))(base() + rva))(c); };
        float attack = ((float (*)(cube::Creature *, bool))(base() + 0x4FA70))(attacker, false);
        float spell = ((float (*)(cube::Creature *, bool))(base() + 0x65C70))(attacker, false);
        int maxCombo = ((int (*)(cube::Creature *))(base() + MAX_COMBO_RVA))(attacker);
        sprintf_s(text,
                  "damage=%.4f critical=%d flag=%d charge=%.4f type=%d ability=%d | attacker id=%lld race=%u level=%u "
                  "ability=%d combo=%d maxcombo=%d attack=%.4f spell=%.4f chargefield=%.4f f18c=%.4f flags2=%08x | "
                  "target id=%lld race=%u level=%u armor=%.4f resistance=%.4f",
                  damage, critical, flag, charge, type, ability, (long long)attacker->id, attacker->entity_data.race,
                  attacker->entity_data.level, a[0x68], *(int *)(a + 0x70), maxCombo, attack, spell,
                  *(float *)(a + 0xba8), *(float *)(a + 0x18c), *(uint32_t *)(a + 0x80), (long long)target->id,
                  target->entity_data.race, target->entity_data.level, stat(0x4F080, target), stat(0x64D90, target));
    }
    std::vector<uint8_t> *const resultsHere = results, *const spansHere = spans;
    const uint32_t drawsBefore = drawsSeen ? (uint32_t)drawsSeen->size() : 0;
    char result = recorder ? recorder(world, target, attacker, damage, critical, flag, charge, where, push, type, ability, extra, last)
                           : original(world, target, attacker, damage, critical, flag, charge, where, push, type, ability, extra, last);
    if (resultsHere)
        resultsHere->push_back((uint8_t)result);
    if (spansHere) {
        const uint32_t drawsAfter = drawsSeen ? (uint32_t)drawsSeen->size() : 0;
        spansHere->insert(spansHere->end(), (const uint8_t *)&drawsBefore, (const uint8_t *)&drawsBefore + 4);
        spansHere->insert(spansHere->end(), (const uint8_t *)&drawsAfter, (const uint8_t *)&drawsAfter + 4);
        spansHere->push_back((uint8_t)result);
    }
    if (log && target) {
        fprintf(log, "t=%llu %s | hp=%.4f->%.4f taken=%.4f applied=%d\n", (unsigned long long)GetTickCount64(), text,
                before, target->entity_data.HP, before - target->entity_data.HP, (int)result);
        fflush(log);
    }
    return result;
}

static void install() {
    if (!supported())
        return;
    auto *entry = (uint8_t *)(base() + APPLY_HIT_RVA);
    // mov rax, rsp; push rbp, rbx, rsi, rdi, r12, r13, r14, r15: 15 bytes with no
    // address of their own, so they run unchanged from the trampoline.
    static const uint8_t prologue[15] = {0x48, 0x8b, 0xc4, 0x55, 0x53, 0x56, 0x57, 0x41,
                                         0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57};
    if (memcmp(entry, prologue, sizeof prologue) != 0) {
        extract::log("Hit probe: unexpected code at the hit function; not installed");
        return;
    }
    auto *tramp = (uint8_t *)VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!tramp)
        return;
    auto absoluteJump = [](uint8_t *at, uintptr_t to) {
        at[0] = 0xff, at[1] = 0x25, *(uint32_t *)(at + 2) = 0; // jmp [rip+0]
        *(uintptr_t *)(at + 6) = to;
    };
    memcpy(tramp, prologue, sizeof prologue);
    absoluteJump(tramp + sizeof prologue, (uintptr_t)entry + sizeof prologue);
    original = (ApplyHit)tramp;
    CreateDirectoryA("Extract", nullptr);
    log = _fsopen("Extract\\hits.txt", "a", _SH_DENYNO);
    DWORD old;
    if (!VirtualProtect(entry, sizeof prologue, PAGE_EXECUTE_READWRITE, &old))
        return;
    absoluteJump(entry, (uintptr_t)&hook);
    entry[14] = 0x90;
    VirtualProtect(entry, sizeof prologue, old, &old);
    FlushInstructionCache(GetCurrentProcess(), entry, sizeof prologue);
}
} // namespace hitprobe

// --- Block writer probe ----------------------------------------------------------------
// Every block the zone generator stores goes through 0x31c1c0 (zone, x, y, z, Block*).
// For the structure types (trunk 7, leaves 8, snowy leaves 11, ...) the call stack is
// recorded, so the code that makes trees can be found; key O writes the tallies to
// Extract\block-writers.txt. Read-only: the original runs unchanged.
namespace blockprobe {
constexpr uintptr_t SET_BLOCK_RVA = 0x31c1c0;
using SetBlock = void (*)(void *, int, int, int, uint8_t *);
static SetBlock original = nullptr;
static std::mutex mutex;
static std::map<std::string, long long> stacks;

static void hook(void *zone, int x, int y, int z, uint8_t *block) {
    if (block && block[4] != 0 && block[4] != 2 && block[4] != 3 && block[4] != 4 && block[4] != 5 && block[4] != 6 &&
        block[4] != 10 && block[4] != 13 && block[4] != 14 && block[4] != 17 && block[4] != 18 && block[4] != 19) {
        void *frames[8];
        USHORT n = RtlCaptureStackBackTrace(1, 8, frames, nullptr);
        char key[256];
        int at = sprintf_s(key, "type %d:", block[4]);
        for (USHORT i = 0; i < n && at < 230; i++) {
            uintptr_t a = (uintptr_t)frames[i];
            at += a >= base() && a < base() + 0x600000 ? sprintf_s(key + at, sizeof key - at, " %llx", (unsigned long long)(a - base()))
                                                         : sprintf_s(key + at, sizeof key - at, " ?");
        }
        std::lock_guard lock(mutex);
        stacks[key]++;
    }
    original(zone, x, y, z, block);
}

static void dump() {
    FILE *f = _fsopen("Extract\\block-writers.txt", "w", _SH_DENYNO);
    if (!f)
        return;
    std::lock_guard lock(mutex);
    for (auto &[key, n] : stacks)
        fprintf(f, "%lld %s\n", n, key.c_str());
    fclose(f);
}

static void install() {
    if (!supported())
        return;
    auto *entry = (uint8_t *)(base() + SET_BLOCK_RVA);
    // push rbp; push rdi; push r14; push r15; sub rsp, 0xc8: 14 bytes with no address of
    // their own, so they run unchanged from the trampoline.
    static const uint8_t prologue[14] = {0x40, 0x55, 0x57, 0x41, 0x56, 0x41, 0x57, 0x48, 0x81, 0xec, 0xc8, 0x00, 0x00, 0x00};
    if (memcmp(entry, prologue, sizeof prologue) != 0) {
        extract::log("Block probe: unexpected code at the block setter; not installed");
        return;
    }
    auto *tramp = (uint8_t *)VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!tramp)
        return;
    auto absoluteJump = [](uint8_t *at, uintptr_t to) {
        at[0] = 0xff, at[1] = 0x25, *(uint32_t *)(at + 2) = 0; // jmp [rip+0]
        *(uintptr_t *)(at + 6) = to;
    };
    memcpy(tramp, prologue, sizeof prologue);
    absoluteJump(tramp + sizeof prologue, (uintptr_t)entry + sizeof prologue);
    original = (SetBlock)tramp;
    DWORD old;
    if (!VirtualProtect(entry, sizeof prologue, PAGE_EXECUTE_READWRITE, &old))
        return;
    absoluteJump(entry, (uintptr_t)&hook);
    VirtualProtect(entry, sizeof prologue, old, &old);
    FlushInstructionCache(GetCurrentProcess(), entry, sizeof prologue);
}
} // namespace blockprobe

// --- Chat log ------------------------------------------------------------------------
// Everything the game prints in its chat panel (townspeople's lines with their names,
// quest messages) goes through ChatWidget::Print (RVA 0x26bf10: widget, std::wstring*,
// colour). It is hooked and each line appended to Extract\chat.txt (UTF-8) with the
// time. Read-only: the original runs unchanged. Its first two instructions (9 bytes,
// no address of their own) move to a trampoline; a 5-byte jump to a stub placed
// within 2 GB of the game reaches the hook.
namespace chatlog {
constexpr uintptr_t PRINT_RVA = 0x26bf10;
using Print = void (*)(void *, std::wstring *, void *);
static Print original = nullptr;
static FILE *log = nullptr;

static void hook(void *widget, std::wstring *text, void *color) {
    if (log && text) {
        int n = WideCharToMultiByte(CP_UTF8, 0, text->c_str(), (int)text->size(), nullptr, 0, nullptr, nullptr);
        std::string utf8(n, '\0');
        WideCharToMultiByte(CP_UTF8, 0, text->c_str(), (int)text->size(), utf8.data(), n, nullptr, nullptr);
        fprintf(log, "t=%llu %s\n", (unsigned long long)GetTickCount64(), utf8.c_str());
        fflush(log);
    }
    original(widget, text, color);
}

// Executable memory within 2 GB below the game, for a rel32 jump.
static uint8_t *nearBlock(uintptr_t target) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    for (uintptr_t at = (target & ~(uintptr_t)0xffff) - 0x10000; at > target - 0x7ff00000; at -= 0x10000)
        if (void *p = VirtualAlloc((void *)at, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE))
            return (uint8_t *)p;
    return nullptr;
}

static void install() {
    if (!supported())
        return;
    auto *entry = (uint8_t *)(base() + PRINT_RVA);
    static const uint8_t prologue[9] = {0x48, 0x83, 0xec, 0x28, 0xf3, 0x41, 0x0f, 0x10, 0x00}; // sub rsp,28; movss xmm0,[r8]
    if (memcmp(entry, prologue, sizeof prologue) != 0) {
        extract::log("Chat log: unexpected code at ChatWidget::Print; not installed");
        return;
    }
    uint8_t *block = nearBlock((uintptr_t)entry);
    if (!block)
        return;
    auto absoluteJump = [](uint8_t *at, uintptr_t to) {
        at[0] = 0xff, at[1] = 0x25, *(uint32_t *)(at + 2) = 0;
        *(uintptr_t *)(at + 6) = to;
    };
    uint8_t *tramp = block, *stub = block + 32;
    memcpy(tramp, prologue, sizeof prologue);
    absoluteJump(tramp + sizeof prologue, (uintptr_t)entry + sizeof prologue);
    absoluteJump(stub, (uintptr_t)&hook);
    original = (Print)tramp;
    CreateDirectoryA("Extract", nullptr);
    log = _fsopen("Extract\\chat.txt", "a", _SH_DENYNO);
    DWORD old;
    if (!VirtualProtect(entry, sizeof prologue, PAGE_EXECUTE_READWRITE, &old))
        return;
    entry[0] = 0xe9;
    *(int32_t *)(entry + 1) = (int32_t)((intptr_t)stub - ((intptr_t)entry + 5));
    for (int i = 5; i < 9; i++)
        entry[i] = 0x90;
    VirtualProtect(entry, sizeof prologue, old, &old);
    FlushInstructionCache(GetCurrentProcess(), entry, sizeof prologue);
}
} // namespace chatlog

// --- Animation oracle ---------------------------------------------------------------
// 0x17ab0 (78 KB) poses and draws a creature: it works out its parts' rotations and
// shifts from the creature's state and writes the 33 world matrices of
// Creature::AnimationState (creature+0xc38). Write a number N to Extract\anim.request
// and the next N calls are appended to Extract\oracle\anim.bin, each with the whole
// creature as the call finds it, the call's arguments and the model objects its sprite
// slots point at. A creature's next record holds the matrices the call before made.
// Each record is preceded by its size. Read-only: the original runs unchanged.
namespace animprobe {
constexpr uintptr_t POSE_RVA = 0x17ab0;
constexpr size_t CREATURE_SIZE = 0x15f8, MODEL_BYTES = 0xa0;
using Pose = void (*)(void *, void *, void *, float, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                      uint64_t);
static Pose original = nullptr;
static std::atomic<int> remaining{0};
static FILE *out = nullptr;
static std::mutex lock;

// Copies n bytes from p, or zeros where it cannot be read.
static void readSafe(const void *p, void *to, size_t n) {
    __try {
        memcpy(to, p, n);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        memset(to, 0, n);
    }
}

// Values the pose gets from the game's rules, called here so the record has them: the
// current ability's times (0x4f220, 0x4ead0, 0x660d0, 0x5e9f0 with -1), the held item's use time
// (0x108fa0 on creature+0x1d8) and whether buff 0xe is on (0x5e3a0).
constexpr uintptr_t ABILITY_TIME_RVA = 0x4f220, ABILITY_LENGTH_RVA = 0x4ead0, ABILITY_LENGTH2_RVA = 0x660d0, ABILITY_LENGTH3_RVA = 0x5e9f0,
                    USE_TIME_RVA = 0x108fa0, BUFF_RVA = 0x5e3a0, USABLE_RVA = 0x50640, ITEM_MODEL_RVA = 0x173b10;
// The items whose models the pose asks for, and the model ids it asks the world for.
static const int ITEM_SLOTS[9] = {0x1d8, 0x278, 0x318, 0x3b8, 0x458, 0x4f8, 0x598, 0x638, 0x6d8};
static const int MODEL_IDS[11] = {0x400, 0x401, 0xa46, 0xb04, 0xd0a, 0xdeb, 0xe21, 0xe2f, 0xe30, 0xe33, 0xe35};

template <class T> static void put(std::vector<uint8_t> &r, const T &v) {
    const uint8_t *p = (const uint8_t *)&v;
    r.insert(r.end(), p, p + sizeof(T));
}
static void putModel(std::vector<uint8_t> &r, void *model) {
    put(r, (uint64_t)model);
    uint8_t bytes[MODEL_BYTES] = {};
    if (model)
        readSafe(model, bytes, MODEL_BYTES);
    r.insert(r.end(), bytes, bytes + MODEL_BYTES);
}
static int callInt(uintptr_t rva, uint8_t *creature, int arg, bool withArg) {
    __try {
        if (withArg)
            return ((int (*)(void *, int))(base() + rva))(creature, arg);
        return ((int (*)(void *))(base() + rva))(creature);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -0x7fffffff;
    }
}
static int callUseTime(uint8_t *item) {
    __try {
        return ((int (*)(void *))(base() + USE_TIME_RVA))(item);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -0x7fffffff;
    }
}
static void *callPtr2(uintptr_t rva, void *a, void *b) {
    __try {
        return ((void *(*)(void *, void *))(base() + rva))(a, b);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}
static void *callPtr2i(uintptr_t rva, void *a, int b) {
    __try {
        return ((void *(*)(void *, int))(base() + rva))(a, b);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}
static bool callBool2(uintptr_t rva, void *a, void *b) {
    __try {
        return ((bool (*)(void *, void *))(base() + rva))(a, b);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
static void *modelById(void *models, int id) {
    void **begin = nullptr, **end = nullptr;
    readSafe((uint8_t *)models + 0x10, &begin, 8);
    readSafe((uint8_t *)models + 0x18, &end, 8);
    if (id < 0 || !begin || id >= end - begin)
        return nullptr;
    void *m = nullptr;
    readSafe(begin + id, &m, 8);
    return m;
}

// Builds the record of a call; `clock` is where timeGetTime() goes, which the hook reads
// last, just before the game's call (the pose reads it late, for the races that beat).
static std::vector<uint8_t> record(void *anim, void *models, float a4, const uint64_t stack[8], size_t &clock) {
    // "CWA3" record: uint64 creature address, creature bytes, float arg 4, the 8 stack args,
    // 16 bytes at arg 5, 8 bytes at args 7 and 8, the ridden creature (a byte whether there
    // is one and its first 0x200 bytes), timeGetTime(); the rules' values (int
    // ability time, length, length 2, length 3, use time, byte buff 0xe, 3 pad); per item slot a byte
    // usable, 7 pad, its model (uint64 address + MODEL_BYTES); then a count and per model
    // id an int32 id, 4 pad and the model.
    std::vector<uint8_t> r;
    r.reserve(0x2000);
    uint8_t *creature = *(uint8_t **)anim;
    put(r, (uint64_t)creature);
    std::vector<uint8_t> bytes(CREATURE_SIZE);
    readSafe(creature, bytes.data(), CREATURE_SIZE);
    r.insert(r.end(), bytes.begin(), bytes.end());
    put(r, a4);
    for (int k = 0; k < 8; k++)
        put(r, stack[k]);
    uint8_t buf[16];
    readSafe((void *)stack[0], buf, 16);
    r.insert(r.end(), buf, buf + 16);
    for (int k : {2, 3}) {
        readSafe((void *)stack[k], buf, 8);
        r.insert(r.end(), buf, buf + 8);
    }
    // The creature ridden (arg 10): a byte whether there is one, then its first 0x200 bytes.
    put(r, (uint8_t)(stack[5] != 0));
    {
        uint8_t mount[0x200] = {};
        if (stack[5])
            readSafe((void *)stack[5], mount, sizeof mount);
        r.insert(r.end(), mount, mount + sizeof mount);
    }
    clock = r.size();
    put(r, (uint32_t)0);
    put(r, callInt(ABILITY_TIME_RVA, creature, 0, false));
    put(r, callInt(ABILITY_LENGTH_RVA, creature, -1, true));
    put(r, callInt(ABILITY_LENGTH2_RVA, creature, -1, true));
    put(r, callInt(ABILITY_LENGTH3_RVA, creature, -1, true));
    put(r, callUseTime(creature + 0x1d8));
    put(r, (uint8_t)(callPtr2i(BUFF_RVA, creature, 0xe) != nullptr));
    put(r, (uint8_t)0), put(r, (uint16_t)0);
    for (int slot : ITEM_SLOTS) {
        put(r, (uint8_t)callBool2(USABLE_RVA, creature, creature + slot));
        for (int k = 0; k < 7; k++)
            put(r, (uint8_t)0);
        putModel(r, callPtr2(ITEM_MODEL_RVA, models, creature + slot));
    }
    std::vector<int> ids(MODEL_IDS, MODEL_IDS + 11);
    for (int k = 0; k < 8; k++)
        ids.push_back(*(int16_t *)(bytes.data() + 0x90 + 2 * k));
    put(r, (uint32_t)ids.size());
    for (int id : ids) {
        put(r, (int32_t)id), put(r, (int32_t)0);
        putModel(r, modelById(models, id));
    }
    return r;
}

static void writeRecord(const std::vector<uint8_t> &r) {
    std::lock_guard<std::mutex> hold(lock);
    if (out) {
        uint32_t n = (uint32_t)r.size();
        fwrite(&n, 4, 1, out);
        fwrite(r.data(), 1, r.size(), out);
        if (remaining.load() <= 0) {
            fclose(out);
            out = nullptr;
            extract::log("Animation oracle: done");
        }
    }
}

static std::atomic<void *> seenModels{nullptr}; // the world's model list, as the pose gets it

static void hook(void *anim, void *context, void *models, float a4, uint64_t s5, uint64_t s6, uint64_t s7, uint64_t s8,
                 uint64_t s9, uint64_t s10, uint64_t s11, uint64_t s12) {
    seenModels = models;
    if (remaining.load() > 0 && remaining.fetch_sub(1) > 0) {
        const uint64_t stack[8] = {s5, s6, s7, s8, s9, s10, s11, s12};
        size_t clock = 0;
        std::vector<uint8_t> r = record(anim, models, a4, stack, clock);
        const uint32_t now = timeGetTime();
        std::memcpy(r.data() + clock, &now, 4);
        original(anim, context, models, a4, s5, s6, s7, s8, s9, s10, s11, s12);
        writeRecord(r);
        return;
    }
    original(anim, context, models, a4, s5, s6, s7, s8, s9, s10, s11, s12);
}

// Extract\itemmodels.request: the item model lookup (0x173b10) for every category, id and
// material that can matter, as model ids (-1: none), into Extract\oracle\item-models.bin.
// The ranges follow docs/specs/item-models.md §2 and §4.4:
//   * ids 0..0x12f each, and id 0x130 for every id above (the tree compares ids up to 0x127);
//   * materials 0..63 and 0x80..0x8f (category 0x0b compares 0x81..0x83);
//   * the modifier over its whole period, 660 (the remainders 2, 3, 4, 5, 6, 10 and 11);
//   * rarities 0..4, and when they matter, on to the first rarity from which every answer
//     is the same (category 1 id 1 adds the raw rarity byte).
// Category 0x17 is skipped (its models are made on demand). The game freezes while this
// runs (a minute or so).
static constexpr uint32_t ITEM_MODEL_PERIOD = 660;
static constexpr uint32_t ITEM_MODEL_ID_BOUND = 0x130;
static constexpr int ITEM_MODEL_RARITIES = 256;

static void itemModels() {
    void *models = seenModels.load();
    if (!models)
        return;
    void **begin = nullptr, **end = nullptr;
    readSafe((uint8_t *)models + 0x10, &begin, 8);
    readSafe((uint8_t *)models + 0x18, &end, 8);
    if (!begin || end <= begin)
        return;
    // A model listed at several indices is named by the lowest.
    std::unordered_map<void *, int> idOf;
    for (void **p = begin; p < end; p++) {
        void *m = nullptr;
        readSafe(p, &m, 8);
        if (m && !idOf.count(m))
            idOf[m] = (int)(p - begin);
    }
    FILE *f = nullptr;
    if (fopen_s(&f, "Extract\\oracle\\item-models.bin", "wb") != 0 || !f)
        return;
    uint8_t item[0xa0];
    auto lookup = [&](int cat, uint32_t id, int mat, int rarity, uint32_t mod) {
        memset(item, 0, sizeof item);
        item[0] = (uint8_t)cat;
        memcpy(item + 4, &id, 4);
        memcpy(item + 8, &mod, 4);
        item[0x14] = (uint8_t)rarity;
        item[0x1c] = (uint8_t)mat;
        void *m = callPtr2(ITEM_MODEL_RVA, models, item);
        auto it = idOf.find(m);
        return it == idOf.end() ? -1 : it->second;
    };
    std::vector<int> materials;
    for (int mat = 0; mat < 64; mat++)
        materials.push_back(mat);
    for (int mat = 0x80; mat < 0x90; mat++)
        materials.push_back(mat);
    // Few distinct grids: each is written once, and each key names its grid.
    std::map<std::vector<int32_t>, uint32_t> gridIndex; // rarities, modifiers, then the ids
    std::vector<const std::vector<int32_t> *> grids;
    std::vector<std::pair<uint64_t, uint32_t>> keyed; // (cat << 32 | id << 8 | mat, grid)
    std::vector<int32_t> byMod(ITEM_MODEL_PERIOD), byRarity(ITEM_MODEL_RARITIES);
    for (int cat = 0; cat < 0x30; cat++) {
        if (cat == 0x17)
            continue;
        for (uint32_t id = 0; id <= ITEM_MODEL_ID_BOUND; id++)
            for (int mat : materials) {
                bool modVaries = false, rarityVaries = false, any = false;
                for (uint32_t m = 0; m < ITEM_MODEL_PERIOD; m++)
                    modVaries |= (byMod[m] = lookup(cat, id, mat, 0, m)) != byMod[0], any |= byMod[m] >= 0;
                for (int r = 0; r < 5; r++)
                    rarityVaries |= (byRarity[r] = lookup(cat, id, mat, r, 0)) != byRarity[0], any |= byRarity[r] >= 0;
                if (!any)
                    continue; // no model: not written
                // Rows: 1, or up to the rarity from which all answers are the same.
                uint32_t rarities = 1;
                if (rarityVaries) {
                    for (int r = 5; r < ITEM_MODEL_RARITIES; r++)
                        byRarity[r] = lookup(cat, id, mat, r, 0);
                    rarities = ITEM_MODEL_RARITIES;
                    while (rarities > 5 && byRarity[rarities - 2] == byRarity[rarities - 1])
                        rarities--;
                }
                const uint32_t modifiers = modVaries ? ITEM_MODEL_PERIOD : 1;
                std::vector<int32_t> g{(int32_t)rarities, (int32_t)modifiers};
                if (!rarityVaries)
                    g.insert(g.end(), byMod.begin(), byMod.begin() + modifiers);
                else if (!modVaries)
                    g.insert(g.end(), byRarity.begin(), byRarity.begin() + rarities);
                else
                    for (uint32_t r = 0; r < rarities; r++)
                        for (uint32_t m = 0; m < modifiers; m++)
                            g.push_back(lookup(cat, id, mat, (int)r, m));
                auto ins = gridIndex.emplace(std::move(g), (uint32_t)grids.size());
                if (ins.second)
                    grids.push_back(&ins.first->first);
                keyed.push_back({(uint64_t)cat << 32 | (uint64_t)id << 8 | (uint64_t)mat, ins.first->second});
            }
    }
    // "CWI3", u32 modifier period, u32 id bound (its key answers for every id above), u32
    // grids, each: u32 rarities, u32 modifiers (1 or the period), the ids as i32,
    // rarity-major; u32 keys, each: u8 category, u8 material, u16 id, u32 grid.
    fwrite("CWI3", 1, 4, f);
    const uint32_t head[2] = {ITEM_MODEL_PERIOD, ITEM_MODEL_ID_BOUND};
    fwrite(head, 4, 2, f);
    uint32_t n = (uint32_t)grids.size();
    fwrite(&n, 4, 1, f);
    for (const auto *g : grids)
        fwrite(g->data(), 4, g->size(), f);
    n = (uint32_t)keyed.size();
    fwrite(&n, 4, 1, f);
    for (auto &[k, g] : keyed) {
        const uint8_t catMat[2] = {(uint8_t)(k >> 32), (uint8_t)k};
        const uint16_t id = (uint16_t)(k >> 8);
        fwrite(catMat, 1, 2, f);
        fwrite(&id, 2, 1, f);
        fwrite(&g, 4, 1, f);
    }
    const size_t keys = keyed.size(), distinct = grids.size();
    fclose(f);
    extract::log("Item models: " + std::to_string(keys) + " keys, " + std::to_string(distinct) + " grids written");
}

// Called each tick: starts a recording when Extract\anim.request appears.
static void poll() {
    if (GetFileAttributesA("Extract\\itemmodels.request") != INVALID_FILE_ATTRIBUTES && seenModels.load()) {
        DeleteFileA("Extract\\itemmodels.request");
        CreateDirectoryA("Extract\\oracle", nullptr);
        itemModels();
    }
    if (GetFileAttributesA("Extract\\anim.request") == INVALID_FILE_ATTRIBUTES || !original)
        return;
    int n = 600;
    if (FILE *f = nullptr; fopen_s(&f, "Extract\\anim.request", "r") == 0 && f) {
        if (fscanf_s(f, "%d", &n) != 1)
            n = 600;
        fclose(f);
    }
    DeleteFileA("Extract\\anim.request");
    std::lock_guard<std::mutex> hold(lock);
    if (out)
        return;
    CreateDirectoryA("Extract\\oracle", nullptr);
    if (fopen_s(&out, "Extract\\oracle\\anim.bin", "wb") != 0 || !out) {
        out = nullptr;
        return;
    }
    fwrite("CWA3", 1, 4, out);
    remaining = n;
    extract::log("Animation oracle: recording " + std::to_string(n) + " pose calls");
}

static void install() {
    if (!supported())
        return;
    auto *entry = (uint8_t *)(base() + POSE_RVA);
    // push rbp, rbx, rsi, rdi, r12, r13, r14, r15: 13 bytes with no address of their own.
    static const uint8_t prologue[13] = {0x40, 0x55, 0x53, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57};
    if (memcmp(entry, prologue, sizeof prologue) != 0) {
        extract::log("Animation oracle: unexpected code at 0x17ab0; not installed");
        return;
    }
    uint8_t *block = chatlog::nearBlock((uintptr_t)entry);
    if (!block)
        return;
    auto absoluteJump = [](uint8_t *at, uintptr_t to) {
        at[0] = 0xff, at[1] = 0x25, *(uint32_t *)(at + 2) = 0;
        *(uintptr_t *)(at + 6) = to;
    };
    uint8_t *tramp = block, *stub = block + 32;
    memcpy(tramp, prologue, sizeof prologue);
    absoluteJump(tramp + sizeof prologue, (uintptr_t)entry + sizeof prologue);
    absoluteJump(stub, (uintptr_t)&hook);
    original = (Pose)tramp;
    DWORD old;
    if (!VirtualProtect(entry, sizeof prologue, PAGE_EXECUTE_READWRITE, &old))
        return;
    entry[0] = 0xe9;
    *(int32_t *)(entry + 1) = (int32_t)((intptr_t)stub - ((intptr_t)entry + 5));
    for (size_t i = 5; i < sizeof prologue; i++)
        entry[i] = 0x90;
    VirtualProtect(entry, sizeof prologue, old, &old);
    FlushInstructionCache(GetCurrentProcess(), entry, sizeof prologue);
}
} // namespace animprobe

// --- Creature update oracle ----------------------------------------------------------
// The game moves every creature with 0x2bb8e0 (world, creature, float dt): physics,
// collisions with the blocks, the stride the pose reads. Write a number N to
// Extract\update.request while in a world, and the next N calls go to
// Extract\oracle\update.bin: "CWU1", int32 world seed, then per call: uint32 size, uint64
// creature address, uint8 whether it is the local player (world+0x448), uint8 world+0x358,
// 2 pad, float dt, the creature (CREATURE_SIZE bytes) before the call, and after it.
// CubeVulkan's port (src/NativeUpdate.cpp) runs on the "before" and is checked against the
// "after"; the blocks it collides with are the generated world's (seed above).
namespace updateprobe {
// The creature's steps of the world's update (0x2da6c0): its physics, its abilities, the
// step after them and its upkeep. "update.request" picks one ("<count> [rva hex]").
constexpr uintptr_t UPDATE_RVA = 0x2bb8e0, ABILITY_RVA = 0x2c5250, STEP_RVA = 0x2c3710, UPKEEP_RVA = 0x2de4c0;
static std::atomic<uintptr_t> which{UPDATE_RVA};
constexpr size_t CREATURE_SIZE = 0x15f8;
using Update = void(__fastcall *)(void *world, void *creature, float dt);
static Update original = nullptr, originalAbility = nullptr, originalStep = nullptr, originalUpkeep = nullptr;
static std::atomic<int> remaining{0};
static std::mutex lock;
static FILE *out = nullptr;

// rand() as the game imports it (its IAT slot at 0x42cd28): while a recorded update runs
// on this thread, every value it draws is kept.
using Rand = int(__cdecl *)();
static Rand realRand = nullptr;
static thread_local std::vector<int32_t> *draws = nullptr;
static int __cdecl countedRand() {
    const int v = realRand();
    if (draws)
        draws->push_back(v);
    return v;
}
template <class T> static void put(std::vector<uint8_t> &r, const T &v) {
    const auto *b = (const uint8_t *)&v;
    r.insert(r.end(), b, b + sizeof v);
}
static void putBytes(std::vector<uint8_t> &r, const void *p, size_t n) {
    const size_t at = r.size();
    r.resize(at + n);
    animprobe::readSafe(p, &r[at], n);
}
// A std::list's values (node+0x10), as the game keeps its lists.
static std::vector<uint8_t *> listOf(void *head) {
    std::vector<uint8_t *> v;
    if (!head)
        return v;
    uint8_t *n = *(uint8_t **)head;
    for (int guard = 0; n && n != head && guard < 100000; guard++) {
        v.push_back(n);
        n = *(uint8_t **)n;
    }
    return v;
}
static int floorBlock(int64_t v) { return (int)(v >> 16); }

// What the update reads of the world, around the creature as it was before the call.
static void putWorld(std::vector<uint8_t> &r, uint8_t *w, uint8_t *c) {
    const uintptr_t b = base();
    auto blockAt = (uint8_t * (*)(void *, uint8_t *, int, int, int))(b + 0xe9a0);
    auto column = (void *(*)(void *, int, int))(b + 0x935e0);
    auto byId = (uint8_t * (*)(void *, int64_t))(b + 0xe720);
    float time = 0;
    animprobe::readSafe(w + 0x440, &time, 4);
    put(r, time);
    const int64_t px = *(int64_t *)(c + 0x10), py = *(int64_t *)(c + 0x18), pz = *(int64_t *)(c + 0x20);
    // The creature's buffs (0x18 bytes each).
    {
        const auto buffs = listOf(*(void **)(c + 0x980));
        put(r, (uint32_t)buffs.size());
        for (uint8_t *n : buffs)
            putBytes(r, n + 0x10, 0x18);
    }
    // The creatures it may meet: within 40 blocks, their first 0x200 bytes.
    auto nearby = [&](uint8_t *o) {
        const int64_t dx = (*(int64_t *)(o + 0x10) - px) >> 16, dy = (*(int64_t *)(o + 0x18) - py) >> 16;
        return dx > -40 && dx < 40 && dy > -40 && dy < 40;
    };
    {
        std::vector<uint8_t *> list;
        for (uint8_t *n : listOf(*(void **)(w + 0x40)))
            list.push_back(*(uint8_t **)(n + 0x10));
        put(r, (uint32_t)list.size());
        for (uint8_t *o : list) {
            put(r, (uint64_t)o);
            const uint8_t isNear = o != c && nearby(o);
            put(r, isNear);
            if (isNear)
                putBytes(r, o, 0x200);
        }
    }
    {
        std::vector<uint8_t *> list;
        for (uint8_t *n : listOf(*(void **)(w + 0x50))) {
            uint8_t *o = *(uint8_t **)(n + 0x10);
            if (nearby(o))
                list.push_back(o);
        }
        put(r, (uint32_t)list.size());
        for (uint8_t *o : list) {
            put(r, (uint64_t)o);
            putBytes(r, o, 0x200);
        }
    }
    // Effect objects of kind 5 the creature owns.
    {
        const int64_t id = *(int64_t *)(c + 8);
        std::vector<uint8_t *> list;
        for (uint8_t *n : listOf(*(void **)(w + 0x60)))
            if (n[0x7c] == 5 && *(int64_t *)(n + 0x10) == id)
                list.push_back(n);
        put(r, (uint32_t)list.size());
        for (uint8_t *n : list)
            putBytes(r, n + 0x10, 0x20);
    }
    // The creatures its record names: what it rides (0x1a8), its owner (0x1a0), its mount (0x9d8).
    for (size_t at : {(size_t)0x1a8, (size_t)0x1a0, (size_t)0x9d8}) {
        const int64_t id = *(int64_t *)(c + at);
        uint8_t *o = byId(w, id);
        put(r, (uint64_t)o);
        if (o)
            putBytes(r, o, 0x9b0);
    }
    // The blocks around it, and whether their zones are loaded.
    const float sx = *(float *)(c + 0x84), sy = *(float *)(c + 0x88), sz = *(float *)(c + 0x8c);
    const int m = 5;
    const int x0 = floorBlock(px) - (int)(sx * 0.5f) - m, x1 = floorBlock(px) + (int)(sx * 0.5f) + m;
    const int y0 = floorBlock(py) - (int)(sy * 0.5f) - m, y1 = floorBlock(py) + (int)(sy * 0.5f) + m;
    const int z0 = floorBlock(pz) - (int)(sz * 0.5f) - m, z1 = floorBlock(pz) + (int)(sz * 0.5f) + m;
    put(r, x0), put(r, y0), put(r, z0), put(r, x1 - x0 + 1), put(r, y1 - y0 + 1), put(r, z1 - z0 + 1);
    for (int x = x0; x <= x1; x++)
        for (int y = y0; y <= y1; y++) {
            put(r, (uint8_t)(column(w, x, y) != nullptr));
            for (int z = z0; z <= z1; z++) {
                uint8_t out[8] = {};
                blockAt(w, out, x, y, z);
                r.insert(r.end(), out, out + 6);
            }
        }
}

// The game's own server world (world+0x358 clear) and its thread, as last seen.
static std::atomic<uint8_t *> serverWorld{nullptr};
static std::atomic<DWORD> serverThread{0};

// The world's lists a step adds to, with the size of what each holds: aggro notes (0x140),
// buff notes (0x150), calls (0x170), damage numbers (0x90), sounds (0xa0), particles (0xb0).
static const std::pair<size_t, size_t> worldLists[] = {{0x140, 0x18}, {0x150, 0x30}, {0x170, 8},
                                                      {0x90, 0x48},  {0xa0, 0x30},  {0xb0, 0x48}, {0x60, 0x80}}; // (CWU4: and the effect objects)
// What a CWU3 record adds about the creature: its buffs (whole), its maps of creature id to
// a float (+0xb88 aggro, +0xb98 damage) and its id sets (+0x9b8, +0x9c8).
static std::vector<uint8_t *> treeNodes(uint8_t *head) {
    std::vector<uint8_t *> v;
    if (!head)
        return v;
    uint8_t *n = *(uint8_t **)head;
    for (int guard = 0; n && n != head && guard < 100000; guard++) {
        v.push_back(n);
        uint8_t *right = *(uint8_t **)(n + 0x10);
        if (!right[0x19]) {
            n = right;
            while (!(*(uint8_t **)n)[0x19])
                n = *(uint8_t **)n;
        } else {
            uint8_t *q = *(uint8_t **)(n + 8);
            while (!q[0x19] && n == *(uint8_t **)(q + 0x10))
                n = q, q = *(uint8_t **)(n + 8);
            n = q;
        }
    }
    return v;
}
static void putSides(std::vector<uint8_t> &r, uint8_t *c) {
    for (size_t at : {(size_t)0xb88, (size_t)0xb98}) {
        const auto nodes = treeNodes(*(uint8_t **)(c + at));
        put(r, (uint32_t)nodes.size());
        for (uint8_t *n : nodes)
            putBytes(r, n + 0x20, 0xc);
    }
    for (size_t at : {(size_t)0x9b8, (size_t)0x9c8}) {
        const auto nodes = treeNodes(*(uint8_t **)(c + at));
        put(r, (uint32_t)nodes.size());
        for (uint8_t *n : nodes)
            putBytes(r, n + 0x20, 8);
    }
}
static void putLists(std::vector<uint8_t> &r, uint8_t *w, const uint32_t *skip) {
    int k = 0;
    for (auto [at, bytes] : worldLists) {
        const auto nodes = listOf(*(void **)(w + at));
        const uint32_t from = skip ? skip[k] : 0;
        put(r, (uint32_t)nodes.size());
        for (size_t i = from; i < nodes.size(); i++)
            putBytes(r, nodes[i] + 0x10, bytes);
        k++;
    }
}

static void step(uintptr_t rva, Update fn, void *world, void *creature, float dt);
static void hook(void *world, void *creature, float dt) { step(UPDATE_RVA, original, world, creature, dt); }
static void abilityHook(void *world, void *creature, float dt) { step(ABILITY_RVA, originalAbility, world, creature, dt); }
// Extract\interact.request ("<count>"): calls of 0x2c3710 that have interactions to take
// up (+0xab8), into Extract\oracle\interact.bin: "CIN1", then per call (uint32 size first):
// uint8 client, float time of day (world+0xc), the creature (record, buffs), its
// interactions (0xc8 bytes each) and for each of kind 5 the object it names (uint8 found,
// 0xe0 bytes) and the creature at +0xb0 (uint8 found, uint8 class); after the call the
// record, the same objects, and the sounds it made (world+0xa0, 0x30 bytes each).
static std::atomic<int> interactsLeft{0};
static FILE *interactOut = nullptr;
static uint8_t *zoneObject(uint8_t *w, int zx, int zy, int index) {
    auto zoneAt = (uint8_t * (*)(void *, int, int))(base() + 0x16e00);
    uint8_t *zone = zoneAt(w, zx, zy);
    if (!zone || index < 0)
        return nullptr;
    uint8_t *b = *(uint8_t **)(zone + 0x60), *e = *(uint8_t **)(zone + 0x68);
    if (!b || index >= (int)((e - b) / 0xe0))
        return nullptr;
    return b + (size_t)index * 0xe0;
}
static void stepHook(void *world, void *creature, float dt) {
    auto *w = (uint8_t *)world, *c = (uint8_t *)creature;
    const auto interactions = listOf(*(void **)(c + 0xab8));
    if (interactions.empty() || interactsLeft.load() <= 0 || w[0x358]) {
        step(STEP_RVA, originalStep, world, creature, dt);
        return;
    }
    std::vector<uint8_t> r;
    put(r, w[0x358]);
    put(r, *(float *)(w + 0xc));
    putBytes(r, c, CREATURE_SIZE);
    {
        const auto buffs = listOf(*(void **)(c + 0x980));
        put(r, (uint32_t)buffs.size());
        for (uint8_t *n : buffs)
            putBytes(r, n + 0x10, 0x18);
    }
    auto byId = (uint8_t * (*)(void *, int64_t))(base() + 0xe720);
    std::vector<uint8_t *> objects;
    put(r, (uint32_t)interactions.size());
    for (uint8_t *n : interactions) {
        uint8_t *in = n + 0x10;
        putBytes(r, in, 0xc8);
        if (in[0xc4] != 5)
            continue;
        uint8_t *o = zoneObject(w, *(int32_t *)(in + 0xa0), *(int32_t *)(in + 0xa4), *(int32_t *)(in + 0xa8));
        objects.push_back(o);
        put(r, (uint8_t)(o != nullptr));
        if (o)
            putBytes(r, o, 0xe0);
        uint8_t *other = byId(w, *(int64_t *)(in + 0xb0));
        put(r, (uint8_t)(other != nullptr));
        put(r, (uint8_t)(other ? other[0x14c] : 0));
    }
    const size_t sounds = listOf(*(void **)(w + 0xa0)).size();
    step(STEP_RVA, originalStep, world, creature, dt);
    putBytes(r, c, CREATURE_SIZE);
    for (uint8_t *o : objects) {
        put(r, (uint8_t)(o != nullptr));
        if (o)
            putBytes(r, o, 0xe0);
    }
    const auto after = listOf(*(void **)(w + 0xa0));
    put(r, (uint32_t)(after.size() > sounds ? after.size() - sounds : 0));
    for (size_t i = sounds; i < after.size(); i++)
        putBytes(r, after[i] + 0x10, 0x30);
    std::lock_guard<std::mutex> hold(lock);
    if (interactOut && interactsLeft.fetch_sub(1) > 0) {
        const uint32_t size = (uint32_t)r.size();
        fwrite(&size, 4, 1, interactOut);
        fwrite(r.data(), 1, r.size(), interactOut);
        if (interactsLeft.load() <= 0) {
            fclose(interactOut);
            interactOut = nullptr;
            extract::log("Interaction oracle: done");
        }
    }
}
static void upkeepHook(void *world, void *creature, float dt) { step(UPKEEP_RVA, originalUpkeep, world, creature, dt); }

// update.request's optional third word: the abilities (+0x68, hex, comma-separated) whose
// calls are recorded; every call when empty.
static std::atomic<bool> filtered{false};
static bool only[256];
static bool wanted(uint8_t ability) { return !filtered.load() || only[ability]; }

static void step(uintptr_t rva, Update fn, void *world, void *creature, float dt) {
    if (rva == UPDATE_RVA) {
        uint8_t client = 1;
        animprobe::readSafe((uint8_t *)world + 0x358, &client, 1);
        if (!client) {
            serverWorld = (uint8_t *)world;
            serverThread = GetCurrentThreadId();
        }
    }
    if (rva == which.load() && remaining.load() > 0 && wanted(((uint8_t *)creature)[0x68]) && remaining.fetch_sub(1) > 0) {
        auto *w = (uint8_t *)world, *c = (uint8_t *)creature;
        std::vector<uint8_t> r(16 + 2 * CREATURE_SIZE);
        const uint64_t address = (uint64_t)creature;
        memcpy(&r[0], &address, 8);
        void *local = nullptr;
        animprobe::readSafe(w + 0x448, &local, 8);
        r[8] = local == creature;
        animprobe::readSafe(w + 0x358, &r[9], 1);
        memcpy(&r[12], &dt, 4);
        animprobe::readSafe(creature, &r[16], CREATURE_SIZE);
        putWorld(r, w, c);
        // CWU3: the creature's sides and the world's lists before the call.
        std::vector<uint8_t> extra;
        {
            const auto buffs = listOf(*(void **)(c + 0x980));
            put(extra, (uint32_t)buffs.size());
            for (uint8_t *n : buffs)
                putBytes(extra, n + 0x10, 0x18);
        }
        putSides(extra, c);
        uint32_t counts[std::size(worldLists)];
        for (size_t k = 0; k < std::size(worldLists); k++)
            counts[k] = (uint32_t)listOf(*(void **)(w + worldLists[k].first)).size();
        putLists(extra, w, nullptr);
        std::vector<int32_t> drawn;
        std::vector<uint8_t> hits, spans;
        draws = &drawn, hitprobe::calls = &hits, hitprobe::drawsSeen = &drawn, hitprobe::spans = &spans;
        fn(world, creature, dt);
        draws = nullptr, hitprobe::calls = nullptr, hitprobe::drawsSeen = nullptr, hitprobe::spans = nullptr;
        animprobe::readSafe(creature, &r[16 + CREATURE_SIZE], CREATURE_SIZE);
        put(r, (uint32_t)drawn.size());
        for (int32_t v : drawn)
            put(r, v);
        // ... and after it: the buffs, sides, what the lists gained and the hits applied.
        {
            const auto buffs = listOf(*(void **)(c + 0x980));
            put(extra, (uint32_t)buffs.size());
            for (uint8_t *n : buffs)
                putBytes(extra, n + 0x10, 0x18);
        }
        putSides(extra, c);
        putLists(extra, w, counts);
        put(extra, (uint32_t)(hits.size() / 79));
        extra.insert(extra.end(), hits.begin(), hits.end());
        // The world's dead (world+0x20: ids), and the creature's zone's linked zones
        // (zone+0x470: keys) with the ids each of those lists (zone+0x480).
        {
            const auto dead = treeNodes(*(uint8_t **)(w + 0x20));
            put(extra, (uint32_t)dead.size());
            for (uint8_t *n : dead)
                putBytes(extra, n + 0x20, 8);
            auto zoneAt = [&](int64_t key) -> uint8_t * {
                for (uint8_t *n : listOf(*(void **)(w + 0x468)))
                    if (*(int64_t *)(n + 0x10) == key)
                        return *(uint8_t **)(n + 0x18);
                return nullptr;
            };
            auto floor64 = [](int64_t v) { return (int32_t)((v >> 16) >= 0 ? (v >> 16) / 64 : -((-(v >> 16) + 63) / 64)); };
            const int64_t key = (int64_t)(uint32_t)floor64(*(int64_t *)(c + 0x10)) | (int64_t)floor64(*(int64_t *)(c + 0x18)) << 32;
            uint8_t *zone = zoneAt(key);
            std::vector<uint8_t *> links = zone ? treeNodes(*(uint8_t **)(zone + 0x470)) : std::vector<uint8_t *>();
            put(extra, (uint8_t)(zone != nullptr));
            put(extra, (uint32_t)links.size());
            for (uint8_t *n : links) {
                int64_t k;
                memcpy(&k, n + 0x1c, 8);
                put(extra, k);
                uint8_t *other = zoneAt(k);
                const auto ids = other ? listOf(*(void **)(other + 0x480)) : std::vector<uint8_t *>();
                put(extra, (uint8_t)(other != nullptr));
                put(extra, (uint32_t)ids.size());
                for (uint8_t *i : ids)
                    putBytes(extra, i + 0x10, 8);
            }
        }
        // Last, each hit's draws (how many before and after it) and result (9 bytes each).
        put(extra, (uint32_t)(spans.size() / 9));
        extra.insert(extra.end(), spans.begin(), spans.end());
        put(r, (uint32_t)rva);
        put(r, (uint32_t)extra.size());
        r.insert(r.end(), extra.begin(), extra.end());
        std::lock_guard<std::mutex> hold(lock);
        if (out) {
            const uint32_t n = (uint32_t)r.size();
            fwrite(&n, 4, 1, out);
            fwrite(r.data(), 1, r.size(), out);
            if (remaining.load() <= 0) {
                fclose(out);
                out = nullptr;
                extract::log("Update oracle: done");
            }
        }
        return;
    }
    fn(world, creature, dt);
}

// Called each tick: starts a recording when Extract\update.request appears.
static void pollInteract() {
    if (GetFileAttributesA("Extract\\interact.request") == INVALID_FILE_ATTRIBUTES || !originalStep)
        return;
    int n = 200;
    if (FILE *f = nullptr; fopen_s(&f, "Extract\\interact.request", "r") == 0 && f) {
        if (fscanf_s(f, "%d", &n) != 1)
            n = 200;
        fclose(f);
    }
    DeleteFileA("Extract\\interact.request");
    std::lock_guard<std::mutex> hold(lock);
    if (interactOut)
        return;
    CreateDirectoryA("Extract\\oracle", nullptr);
    if (fopen_s(&interactOut, "Extract\\oracle\\interact.bin", "wb") != 0 || !interactOut) {
        interactOut = nullptr;
        return;
    }
    fwrite("CIN1", 1, 4, interactOut);
    interactsLeft = n;
    extract::log("Interaction oracle: recording " + std::to_string(n) + " calls");
}
static void poll(cube::World *w) {
    pollInteract();
    if (GetFileAttributesA("Extract\\update.request") == INVALID_FILE_ATTRIBUTES || !original)
        return;
    int n = 2000;
    unsigned rva = UPDATE_RVA;
    char list[256] = "";
    if (FILE *f = nullptr; fopen_s(&f, "Extract\\update.request", "r") == 0 && f) {
        if (fscanf_s(f, "%d %x %255s", &n, &rva, list, (unsigned)sizeof list) < 1)
            n = 2000;
        fclose(f);
    }
    memset(only, 0, sizeof only);
    filtered = false;
    for (char *p = list; *p;) {
        char *end = nullptr;
        const unsigned long a = strtoul(p, &end, 16);
        if (end == p)
            break;
        only[a & 0xff] = true;
        filtered = true;
        p = *end == ',' ? end + 1 : end;
    }
    if (rva != ABILITY_RVA && rva != STEP_RVA && rva != UPKEEP_RVA)
        rva = UPDATE_RVA;
    DeleteFileA("Extract\\update.request");
    std::lock_guard<std::mutex> hold(lock);
    if (out)
        return;
    CreateDirectoryA("Extract\\oracle", nullptr);
    if (fopen_s(&out, "Extract\\oracle\\update.bin", "wb") != 0 || !out) {
        out = nullptr;
        return;
    }
    fwrite("CWU4", 1, 4, out);
    which = rva;
    const int32_t seed = w ? w->seed : 0;
    fwrite(&seed, 4, 1, out);
    remaining = n;
    extract::log("Update oracle: recording " + std::to_string(n) + " creature updates");
}

static void install() {
    if (!supported())
        return;
    auto *entry = (uint8_t *)(base() + UPDATE_RVA);
    // mov rax, rsp; push rbp, rsi, rdi, r12, r13, r14, r15: 14 bytes, no address of their own.
    static const uint8_t prologue[14] = {0x48, 0x8b, 0xc4, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57};
    if (memcmp(entry, prologue, sizeof prologue) != 0) {
        extract::log("Update oracle: unexpected code at 0x2bb8e0; not installed");
        return;
    }
    uint8_t *block = chatlog::nearBlock((uintptr_t)entry);
    if (!block)
        return;
    {
        auto *slot = (Rand *)(base() + 0x42cd28);
        DWORD was;
        if (VirtualProtect(slot, sizeof *slot, PAGE_READWRITE, &was)) {
            realRand = *slot;
            *slot = &countedRand;
            VirtualProtect(slot, sizeof *slot, was, &was);
        }
    }
    auto absoluteJump = [](uint8_t *at, uintptr_t to) {
        at[0] = 0xff, at[1] = 0x25, *(uint32_t *)(at + 2) = 0;
        *(uintptr_t *)(at + 6) = to;
    };
    uint8_t *tramp = block, *stub = block + 32;
    memcpy(tramp, prologue, sizeof prologue);
    absoluteJump(tramp + sizeof prologue, (uintptr_t)entry + sizeof prologue);
    absoluteJump(stub, (uintptr_t)&hook);
    original = (Update)tramp;
    // The other steps: mov rax, rsp (or push rbp) and pushes, 12 bytes, none relative.
    struct Step {
        uintptr_t rva;
        uint8_t start[12];
        Update *keep;
        void (*to)(void *, void *, float);
    } more[] = {{ABILITY_RVA, {0x40, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57}, &originalAbility, &abilityHook},
                {STEP_RVA, {0x48, 0x8b, 0xc4, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56}, &originalStep, &stepHook},
                {UPKEEP_RVA, {0x48, 0x8b, 0xc4, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56}, &originalUpkeep, &upkeepHook}};
    for (Step &st : more) {
        auto *at = (uint8_t *)(base() + st.rva);
        uint8_t *blk = memcmp(at, st.start, sizeof st.start) == 0 ? chatlog::nearBlock((uintptr_t)at) : nullptr;
        if (!blk) {
            extract::log("Update oracle: no hook at 0x" + std::to_string(st.rva));
            continue;
        }
        memcpy(blk, st.start, sizeof st.start);
        absoluteJump(blk + sizeof st.start, (uintptr_t)at + sizeof st.start);
        absoluteJump(blk + 32, (uintptr_t)st.to);
        *st.keep = (Update)blk;
        DWORD was;
        if (!VirtualProtect(at, sizeof st.start, PAGE_EXECUTE_READWRITE, &was))
            continue;
        at[0] = 0xe9;
        *(int32_t *)(at + 1) = (int32_t)((intptr_t)(blk + 32) - ((intptr_t)at + 5));
        for (size_t i = 5; i < sizeof st.start; i++)
            at[i] = 0x90;
        VirtualProtect(at, sizeof st.start, was, &was);
        FlushInstructionCache(GetCurrentProcess(), at, sizeof st.start);
    }
    DWORD old;
    if (!VirtualProtect(entry, sizeof prologue, PAGE_EXECUTE_READWRITE, &old))
        return;
    entry[0] = 0xe9;
    *(int32_t *)(entry + 1) = (int32_t)((intptr_t)stub - ((intptr_t)entry + 5));
    for (size_t i = 5; i < sizeof prologue; i++)
        entry[i] = 0x90;
    VirtualProtect(entry, sizeof prologue, old, &old);
    FlushInstructionCache(GetCurrentProcess(), entry, sizeof prologue);
}
} // namespace updateprobe

// --- Hit oracle --------------------------------------------------------------------------
// Writing "<count>" to Extract\hit.request records the next calls of the hit 0x29eb60 (and
// so of 0x29dcf0 that it calls) into Extract\oracle\hit.bin: "CHT1", then per call: uint32
// size; uint8 world+0x358, whether the target and the attacker are the local player,
// whether the world has one, the result, 3 pad; the call's arguments (79 bytes, as
// hitprobe::calls has them); uint8 whether there is an attacker; the target (and
// attacker) records, buffs and sides; every creature of the world (address, its first
// 0x200 bytes, +0x9e0, its aggro map); the world's lists' sizes; then the same after the
// call, with what the lists gained (their sizes before, then after with the new entries;
// the kills, world+0x120, 16 bytes each), and every rand() it drew. CubeVulkan's port (src/NativeHit.cpp) is checked against it.
namespace hitoracle {
using updateprobe::put;
using updateprobe::putBytes;
constexpr size_t CREATURE_SIZE = updateprobe::CREATURE_SIZE;
static std::atomic<int> remaining{0};
static std::mutex lock;
static FILE *out = nullptr;

static void putBuffs(std::vector<uint8_t> &r, uint8_t *c) {
    const auto buffs = updateprobe::listOf(*(void **)(c + 0x980));
    put(r, (uint32_t)buffs.size());
    for (uint8_t *n : buffs)
        putBytes(r, n + 0x10, 0x18);
}
static void putCreatures(std::vector<uint8_t> &r, uint8_t *w) {
    const auto list = updateprobe::listOf(*(void **)(w + 0x40));
    put(r, (uint32_t)list.size());
    for (uint8_t *n : list) {
        uint8_t *o = *(uint8_t **)(n + 0x10);
        put(r, (uint64_t)o);
        putBytes(r, o, 0x200);
        putBytes(r, o + 0x9e0, 8);
        const auto aggro = updateprobe::treeNodes(*(uint8_t **)(o + 0xb88));
        put(r, (uint32_t)aggro.size());
        for (uint8_t *a : aggro)
            putBytes(r, a + 0x20, 0xc);
    }
}
static const size_t lists[] = {0x140, 0x150, 0x170, 0x90, 0xa0, 0xb0, 0x120};
static const size_t listBytes[] = {0x18, 0x30, 8, 0x48, 0x30, 0x48, 0x10};

static char record(void *world, cube::Creature *target, cube::Creature *attacker, float damage, uint8_t critical,
                   uint8_t flag, float charge, void *where, void *push, int type, int ability, int64_t extra,
                   uint8_t last) {
    auto call = [&] {
        return hitprobe::original(world, target, attacker, damage, critical, flag, charge, where, push, type, ability,
                                  extra, last);
    };
    if (!target || remaining.load() <= 0 || remaining.fetch_sub(1) <= 0)
        return call();
    auto *w = (uint8_t *)world, *t = (uint8_t *)target, *a = (uint8_t *)attacker;
    std::vector<uint8_t> r(8);
    const uint8_t *local = *(uint8_t **)(w + 0x448);
    r[0] = w[0x358], r[1] = t == local, r[2] = a && a == local, r[3] = local != nullptr;
    {
        const int64_t tid = *(int64_t *)(t + 8), aid = a ? *(int64_t *)(a + 8) : 0;
        put(r, tid), put(r, aid), put(r, damage), put(r, critical), put(r, flag), put(r, charge);
        int64_t at[3] = {};
        float push3[3] = {};
        if (where)
            memcpy(at, where, sizeof at);
        if (push)
            memcpy(push3, push, sizeof push3);
        putBytes(r, at, sizeof at), putBytes(r, push3, sizeof push3);
        put(r, type), put(r, ability), put(r, extra), put(r, last);
    }
    put(r, (uint8_t)(a != nullptr));
    auto side = [&](uint8_t *c) {
        putBytes(r, c, CREATURE_SIZE);
        putBuffs(r, c);
        updateprobe::putSides(r, c);
    };
    side(t);
    if (a)
        side(a);
    putCreatures(r, w);
    uint32_t counts[std::size(lists)];
    for (size_t k = 0; k < std::size(lists); k++) {
        counts[k] = (uint32_t)updateprobe::listOf(*(void **)(w + lists[k])).size();
        put(r, counts[k]);
    }
    std::vector<int32_t> drawn, *was = updateprobe::draws;
    updateprobe::draws = &drawn;
    const char result = call();
    updateprobe::draws = was;
    if (was)
        was->insert(was->end(), drawn.begin(), drawn.end());
    r[4] = (uint8_t)result;
    side(t);
    if (a)
        side(a);
    putCreatures(r, w);
    for (size_t k = 0; k < std::size(lists); k++) {
        const auto nodes = updateprobe::listOf(*(void **)(w + lists[k]));
        put(r, (uint32_t)nodes.size());
        for (size_t i = counts[k]; i < nodes.size(); i++)
            putBytes(r, nodes[i] + 0x10, listBytes[k]);
    }
    put(r, (uint32_t)drawn.size());
    for (int32_t v : drawn)
        put(r, v);
    std::lock_guard<std::mutex> hold(lock);
    if (out) {
        const uint32_t n = (uint32_t)r.size();
        fwrite(&n, 4, 1, out);
        fwrite(r.data(), 1, r.size(), out);
        if (remaining.load() <= 0) {
            fclose(out);
            out = nullptr;
            extract::log("Hit oracle: done");
        }
    }
    return result;
}

static void poll() {
    if (GetFileAttributesA("Extract\\hit.request") == INVALID_FILE_ATTRIBUTES || !hitprobe::original)
        return;
    int n = 300;
    if (FILE *f = nullptr; fopen_s(&f, "Extract\\hit.request", "r") == 0 && f) {
        if (fscanf_s(f, "%d", &n) != 1)
            n = 300;
        fclose(f);
    }
    DeleteFileA("Extract\\hit.request");
    std::lock_guard<std::mutex> hold(lock);
    if (out)
        return;
    CreateDirectoryA("Extract\\oracle", nullptr);
    if (fopen_s(&out, "Extract\\oracle\\hit.bin", "wb") != 0 || !out) {
        out = nullptr;
        return;
    }
    fwrite("CHT1", 1, 4, out);
    remaining = n;
    hitprobe::recorder = &record;
    extract::log("Hit oracle: recording " + std::to_string(n) + " hits");
}
} // namespace hitoracle

// --- Effect oracle ------------------------------------------------------------------------
// Writing "<count>" to Extract\effects.request records the next calls of the effect-object
// step 0x2dba40 (world, dt) that have effect objects, into Extract\oracle\effects.bin:
// "CFX1", then per call: uint32 size; uint8 world+0x358, whether it has a local player, 2
// pad, float dt; the effect objects (0x80 bytes each); every creature (address, whether it
// counts whole: it owns an effect or stands within 40 blocks of one; then the whole record
// or its first 0x200 bytes, its buffs, and when whole its sides); the blocks around each effect (radius 6: corner, size, then
// per column whether it is loaded and its blocks); the world's lists' sizes; then after the
// call the effects, the creatures again, what the lists gained, each hit asked for with
// its result, and every rand() drawn. CubeVulkan's port (src/NativeEffects.cpp) is checked
// against it.
namespace effectoracle {
using updateprobe::put;
using updateprobe::putBytes;
constexpr size_t CREATURE_SIZE = updateprobe::CREATURE_SIZE;
constexpr uintptr_t STEP_RVA = 0x2dba40;
using Step = void(__fastcall *)(void *world, float dt);
static Step original = nullptr;
static std::atomic<int> remaining{0};
static std::mutex lock;
static FILE *out = nullptr;
static const size_t lists[] = {0xa0, 0xb0, 0x90, 0x150, 0x140, 0xc0};
static const size_t listBytes[] = {0x30, 0x48, 0x48, 0x30, 0x18, 0x20};

// A creature counts whole (record and sides) when it owns an effect or stands within 40
// blocks of one (a heal's target, those whose anger a heal changes); others by their head.
static void putCreatures(std::vector<uint8_t> &r, uint8_t *w, const std::set<int64_t> &owners,
                         const std::vector<std::array<int64_t, 3>> &places) {
    const auto list = updateprobe::listOf(*(void **)(w + 0x40));
    put(r, (uint32_t)list.size());
    for (uint8_t *n : list) {
        uint8_t *o = *(uint8_t **)(n + 0x10);
        bool owner = owners.count(*(int64_t *)(o + 8)) > 0;
        for (auto &p : places)
            if (std::llabs((*(int64_t *)(o + 0x10) - p[0]) >> 16) < 40 && std::llabs((*(int64_t *)(o + 0x18) - p[1]) >> 16) < 40)
                owner = true;
        put(r, (uint64_t)o);
        put(r, (uint8_t)owner);
        putBytes(r, o, owner ? CREATURE_SIZE : 0x200);
        const auto buffs = updateprobe::listOf(*(void **)(o + 0x980));
        put(r, (uint32_t)buffs.size());
        for (uint8_t *b : buffs)
            putBytes(r, b + 0x10, 0x18);
        if (owner)
            updateprobe::putSides(r, o);
    }
}

static void hook(void *world, float dt) {
    auto *w = (uint8_t *)world;
    const auto effects = updateprobe::listOf(*(void **)(w + 0x60));
    if (effects.empty() || remaining.load() <= 0 || remaining.fetch_sub(1) <= 0) {
        original(world, dt);
        return;
    }
    std::vector<uint8_t> r(8);
    r[0] = w[0x358], r[1] = *(uint8_t **)(w + 0x448) != nullptr;
    memcpy(&r[4], &dt, 4);
    std::set<int64_t> owners;
    std::vector<std::array<int64_t, 3>> places;
    put(r, (uint32_t)effects.size());
    for (uint8_t *n : effects) {
        putBytes(r, n + 0x10, 0x80);
        owners.insert(*(int64_t *)(n + 0x10));
        places.push_back({*(int64_t *)(n + 0x18), *(int64_t *)(n + 0x20), *(int64_t *)(n + 0x28)});
    }
    putCreatures(r, w, owners, places);
    {
        auto blockAt = (uint8_t * (*)(void *, uint8_t *, int, int, int))(base() + 0xe9a0);
        auto column = (void *(*)(void *, int, int))(base() + 0x935e0);
        put(r, (uint32_t)effects.size());
        for (uint8_t *n : effects) {
            const int m = 6;
            const int x0 = (int)(*(int64_t *)(n + 0x18) >> 16) - m, y0 = (int)(*(int64_t *)(n + 0x20) >> 16) - m,
                      z0 = (int)(*(int64_t *)(n + 0x28) >> 16) - m;
            const int size = 2 * m + 1;
            put(r, x0), put(r, y0), put(r, z0), put(r, size);
            for (int x = x0; x < x0 + size; x++)
                for (int y = y0; y < y0 + size; y++) {
                    put(r, (uint8_t)(column(w, x, y) != nullptr));
                    for (int z = z0; z < z0 + size; z++) {
                        uint8_t b[8] = {};
                        blockAt(w, b, x, y, z);
                        r.insert(r.end(), b, b + 6);
                    }
                }
        }
    }
    uint32_t counts[std::size(lists)];
    for (size_t k = 0; k < std::size(lists); k++) {
        counts[k] = (uint32_t)updateprobe::listOf(*(void **)(w + lists[k])).size();
        put(r, counts[k]);
    }
    std::vector<int32_t> drawn, *was = updateprobe::draws;
    std::vector<uint8_t> hits, *wasHits = hitprobe::calls;
    std::vector<uint8_t> results, *wasResults = hitprobe::results;
    updateprobe::draws = &drawn, hitprobe::calls = &hits, hitprobe::results = &results;
    original(world, dt);
    updateprobe::draws = was, hitprobe::calls = wasHits, hitprobe::results = wasResults;
    const auto after = updateprobe::listOf(*(void **)(w + 0x60));
    put(r, (uint32_t)after.size());
    for (uint8_t *n : after)
        putBytes(r, n + 0x10, 0x80);
    putCreatures(r, w, owners, places);
    for (size_t k = 0; k < std::size(lists); k++) {
        const auto nodes = updateprobe::listOf(*(void **)(w + lists[k]));
        put(r, (uint32_t)nodes.size());
        for (size_t i = counts[k]; i < nodes.size(); i++)
            putBytes(r, nodes[i] + 0x10, listBytes[k]);
    }
    put(r, (uint32_t)(hits.size() / 79));
    r.insert(r.end(), hits.begin(), hits.end());
    r.insert(r.end(), results.begin(), results.end());
    put(r, (uint32_t)drawn.size());
    for (int32_t v : drawn)
        put(r, v);
    std::lock_guard<std::mutex> hold(lock);
    if (out) {
        const uint32_t size = (uint32_t)r.size();
        fwrite(&size, 4, 1, out);
        fwrite(r.data(), 1, r.size(), out);
        if (remaining.load() <= 0) {
            fclose(out);
            out = nullptr;
            extract::log("Effect oracle: done");
        }
    }
}

static void poll() {
    if (GetFileAttributesA("Extract\\effects.request") == INVALID_FILE_ATTRIBUTES || !original)
        return;
    int n = 300;
    if (FILE *f = nullptr; fopen_s(&f, "Extract\\effects.request", "r") == 0 && f) {
        if (fscanf_s(f, "%d", &n) != 1)
            n = 300;
        fclose(f);
    }
    DeleteFileA("Extract\\effects.request");
    std::lock_guard<std::mutex> hold(lock);
    if (out)
        return;
    CreateDirectoryA("Extract\\oracle", nullptr);
    if (fopen_s(&out, "Extract\\oracle\\effects.bin", "wb") != 0 || !out) {
        out = nullptr;
        return;
    }
    fwrite("CFX1", 1, 4, out);
    remaining = n;
    extract::log("Effect oracle: recording " + std::to_string(n) + " effect steps");
}

static void install() {
    if (!supported())
        return;
    auto *at = (uint8_t *)(base() + STEP_RVA);
    // mov rax, rsp; push rbp, rsi, rdi, r12, r13, r14, r15: 14 bytes, none relative.
    static const uint8_t start[14] = {0x48, 0x8b, 0xc4, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57};
    uint8_t *blk = memcmp(at, start, sizeof start) == 0 ? chatlog::nearBlock((uintptr_t)at) : nullptr;
    if (!blk) {
        extract::log("Effect oracle: unexpected code at 0x2dba40; not installed");
        return;
    }
    auto absoluteJump = [](uint8_t *p, uintptr_t to) {
        p[0] = 0xff, p[1] = 0x25, *(uint32_t *)(p + 2) = 0;
        *(uintptr_t *)(p + 6) = to;
    };
    memcpy(blk, start, sizeof start);
    absoluteJump(blk + sizeof start, (uintptr_t)at + sizeof start);
    absoluteJump(blk + 32, (uintptr_t)&hook);
    original = (Step)blk;
    DWORD was;
    if (!VirtualProtect(at, sizeof start, PAGE_EXECUTE_READWRITE, &was))
        return;
    at[0] = 0xe9;
    *(int32_t *)(at + 1) = (int32_t)((intptr_t)(blk + 32) - ((intptr_t)at + 5));
    for (size_t i = 5; i < sizeof start; i++)
        at[i] = 0x90;
    VirtualProtect(at, sizeof start, was, &was);
    FlushInstructionCache(GetCurrentProcess(), at, sizeof start);
}
} // namespace effectoracle

// --- Drop oracle (Extract\drops.request) ----------------------------------------------
// What a death drops (0x2a6860) and how each drop is placed (0x2a5b70). "drops.request"
// holds a count of deaths to record into Extract\oracle\drops.bin: "CDR1", then per death
// (uint32 size first): the world's time (world+8), the dead creature (record, inventory
// tabs, aggro), every creature (id, hostility, and the whole record of players), the
// world's special-item lists (world+0x360, +0x378), the block it died in, each call to
// 0x2a5b70 (its arguments, whether the zone is loaded, the blocks of the column it drops
// down, and the drops it laid in the zone) and every rand() drawn. Read-only for the game.
namespace droporacle {
using updateprobe::put;
using updateprobe::putBytes;
constexpr size_t CREATURE_SIZE = updateprobe::CREATURE_SIZE;
constexpr uintptr_t DROP_RVA = 0x2a6860, PLACE_RVA = 0x2a5b70;
using Drop = void(__fastcall *)(void *world, uint8_t *dead);
using Place = void(__fastcall *)(void *world, uint8_t *item, int64_t *pos, float angle, float size, int kind,
                                 uint8_t *from, int64_t owner);
static Drop originalDrop = nullptr;
static Place originalPlace = nullptr;
static std::atomic<int> remaining{0};
static std::mutex lock;
static FILE *out = nullptr;
static thread_local std::vector<uint8_t> *calls = nullptr;
static thread_local uint32_t callCount = 0;

static uint8_t *zoneAt(uint8_t *w, int zx, int zy) {
    auto find = (void **(*)(void *, void **, int64_t *))(base() + 0x12700);
    void *node = nullptr;
    int64_t key = ((int64_t)(uint32_t)zy << 32) | (uint32_t)zx;
    find(w + 0x460, &node, &key);
    if (!node || node == *(void **)(w + 0x468))
        return nullptr;
    return *(uint8_t **)((uint8_t *)node + 0x18);
}

static void placeHook(void *world, uint8_t *item, int64_t *pos, float angle, float size, int kind, uint8_t *from,
                      int64_t owner) {
    if (!calls) {
        originalPlace(world, item, pos, angle, size, kind, from, owner);
        return;
    }
    auto *w = (uint8_t *)world;
    auto &r = *calls;
    callCount++;
    putBytes(r, item, 0xa0);
    putBytes(r, pos, 24);
    put(r, angle), put(r, size), put(r, kind);
    put(r, (uint64_t)from), put(r, owner);
    const int bx = (int)(pos[0] >> 16), by = (int)(pos[1] >> 16), bz = (int)(pos[2] >> 16);
    uint8_t *zone = zoneAt(w, bx >> 6, by >> 6);
    put(r, (uint8_t)(zone != nullptr));
    auto blockAt = (uint8_t * (*)(void *, uint8_t *, int, int, int))(base() + 0xe9a0);
    std::vector<uint8_t> column;
    uint32_t n = 0;
    for (int z = bz; n < 512; z--, n++) {
        uint8_t b[8] = {};
        blockAt(w, b, bx, by, z);
        column.insert(column.end(), b, b + 6);
        if (b[4] & 0xfd) {
            n++;
            break;
        }
    }
    put(r, bz), put(r, n);
    r.insert(r.end(), column.begin(), column.end());
    const size_t before = zone ? (size_t)((*(uint8_t **)(zone + 0x38) - *(uint8_t **)(zone + 0x30)) / 0xe0) : 0;
    auto *was = calls;
    calls = nullptr; // (0x2a5b70 calls nothing that drops)
    originalPlace(world, item, pos, angle, size, kind, from, owner);
    calls = was;
    if (!zone) {
        put(r, (uint32_t)0);
        return;
    }
    uint8_t *begin = *(uint8_t **)(zone + 0x30);
    const size_t after = (size_t)((*(uint8_t **)(zone + 0x38) - begin) / 0xe0);
    put(r, (uint32_t)(after > before ? after - before : 0));
    for (size_t i = before; i < after; i++)
        putBytes(r, begin + i * 0xe0, 0xe0);
}

static void dropHook(void *world, uint8_t *dead) {
    if (remaining.load() <= 0 || calls || remaining.fetch_sub(1) <= 0) {
        originalDrop(world, dead);
        return;
    }
    auto *w = (uint8_t *)world;
    std::vector<uint8_t> r;
    put(r, *(int32_t *)(w + 8));
    put(r, (uint64_t)dead);
    putBytes(r, dead, CREATURE_SIZE);
    {
        auto *tabs = *(uint8_t **)(dead + 0x9f0), *end = *(uint8_t **)(dead + 0x9f8);
        put(r, (uint32_t)(tabs ? (end - tabs) / 0x18 : 0));
        for (uint8_t *t = tabs; t && t < end; t += 0x18) {
            auto *b = *(uint8_t **)t, *e = *(uint8_t **)(t + 8);
            const uint32_t n = b ? (uint32_t)((e - b) / 0xa4) : 0;
            put(r, n);
            putBytes(r, b, n * 0xa4);
        }
        const auto aggro = updateprobe::treeNodes(*(uint8_t **)(dead + 0xb88));
        put(r, (uint32_t)aggro.size());
        for (uint8_t *a : aggro)
            putBytes(r, a + 0x20, 0xc);
    }
    const auto list = updateprobe::listOf(*(void **)(w + 0x40));
    put(r, (uint32_t)list.size());
    for (uint8_t *node : list) {
        uint8_t *o = *(uint8_t **)(node + 0x10);
        put(r, (uint64_t)o), put(r, *(int64_t *)(o + 8)), put(r, o[0x60]);
        if (o[0x60] == 0)
            putBytes(r, o, CREATURE_SIZE);
    }
    {
        auto *b = *(int32_t **)(w + 0x360), *e = *(int32_t **)(w + 0x368);
        const uint32_t n = b ? (uint32_t)(e - b) : 0;
        put(r, n);
        putBytes(r, b, n * 4);
        const auto nodes = updateprobe::treeNodes(*(uint8_t **)(w + 0x378));
        put(r, (uint32_t)nodes.size());
        for (uint8_t *node : nodes) {
            auto *vb = *(int32_t **)(node + 0x28), *ve = *(int32_t **)(node + 0x30);
            const uint32_t m = vb ? (uint32_t)(ve - vb) : 0;
            put(r, *(int32_t *)(node + 0x20)), put(r, m);
            putBytes(r, vb, m * 4);
        }
    }
    {
        auto blockAt = (uint8_t * (*)(void *, uint8_t *, int, int, int))(base() + 0xe9a0);
        uint8_t b[8] = {};
        blockAt(w, b, (int)(*(int64_t *)(dead + 0x10) >> 16), (int)(*(int64_t *)(dead + 0x18) >> 16),
                (int)(*(int64_t *)(dead + 0x20) >> 16));
        r.insert(r.end(), b, b + 6);
    }
    std::vector<uint8_t> placed;
    std::vector<int32_t> drawn, *was = updateprobe::draws;
    updateprobe::draws = &drawn;
    calls = &placed, callCount = 0;
    originalDrop(world, dead);
    calls = nullptr;
    updateprobe::draws = was;
    put(r, callCount);
    r.insert(r.end(), placed.begin(), placed.end());
    put(r, (uint32_t)drawn.size());
    for (int32_t v : drawn)
        put(r, v);
    std::lock_guard<std::mutex> hold(lock);
    if (out) {
        const uint32_t size = (uint32_t)r.size();
        fwrite(&size, 4, 1, out);
        fwrite(r.data(), 1, r.size(), out);
        fflush(out);
        extract::log("Drop oracle: recorded a death (" + std::to_string(callCount) + " drops)");
        if (remaining.load() <= 0) {
            fclose(out);
            out = nullptr;
            extract::log("Drop oracle: done");
        }
    }
}

static void poll() {
    if (GetFileAttributesA("Extract\\drops.request") == INVALID_FILE_ATTRIBUTES || !originalDrop)
        return;
    int n = 20;
    if (FILE *f = nullptr; fopen_s(&f, "Extract\\drops.request", "r") == 0 && f) {
        if (fscanf_s(f, "%d", &n) != 1)
            n = 20;
        fclose(f);
    }
    DeleteFileA("Extract\\drops.request");
    std::lock_guard<std::mutex> hold(lock);
    if (out)
        return;
    CreateDirectoryA("Extract\\oracle", nullptr);
    if (fopen_s(&out, "Extract\\oracle\\drops.bin", "wb") != 0 || !out) {
        out = nullptr;
        return;
    }
    fwrite("CDR1", 1, 4, out);
    remaining = n;
    extract::log("Drop oracle: recording " + std::to_string(n) + " deaths");
}

// Both start with mov rax, rsp; push rbp, rsi, rdi, r12, r13, r14, r15 (14 bytes, none relative).
static void *detour(uintptr_t rva, void *to) {
    auto *at = (uint8_t *)(base() + rva);
    static const uint8_t start[14] = {0x48, 0x8b, 0xc4, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57};
    uint8_t *blk = memcmp(at, start, sizeof start) == 0 ? chatlog::nearBlock((uintptr_t)at) : nullptr;
    if (!blk)
        return nullptr;
    auto absoluteJump = [](uint8_t *p, uintptr_t dest) {
        p[0] = 0xff, p[1] = 0x25, *(uint32_t *)(p + 2) = 0;
        *(uintptr_t *)(p + 6) = dest;
    };
    memcpy(blk, start, sizeof start);
    absoluteJump(blk + sizeof start, (uintptr_t)at + sizeof start);
    absoluteJump(blk + 32, (uintptr_t)to);
    DWORD was;
    if (!VirtualProtect(at, sizeof start, PAGE_EXECUTE_READWRITE, &was))
        return nullptr;
    at[0] = 0xe9;
    *(int32_t *)(at + 1) = (int32_t)((intptr_t)(blk + 32) - ((intptr_t)at + 5));
    for (size_t i = 5; i < sizeof start; i++)
        at[i] = 0x90;
    VirtualProtect(at, sizeof start, was, &was);
    FlushInstructionCache(GetCurrentProcess(), at, sizeof start);
    return blk;
}

static void install() {
    if (!supported())
        return;
    originalPlace = (Place)detour(PLACE_RVA, (void *)&placeHook);
    if (!originalPlace) {
        extract::log("Drop oracle: unexpected code at 0x2a5b70; not installed");
        return;
    }
    originalDrop = (Drop)detour(DROP_RVA, (void *)&dropHook);
    if (!originalDrop)
        extract::log("Drop oracle: unexpected code at 0x2a6860; not installed");
}
} // namespace droporacle

// --- Controller oracle (Extract\control.request) --------------------------------------
// 0x9c050 (game, double dt) moves the local player by the keys each frame. "control.request"
// holds a count N of calls to record into Extract\oracle\control.bin: "CCT1", then per
// call: uint32 size, double dt, game+0x1a70..0x1a80 (the keys), game+0x1d6c and 0x1d70
// (the stick), double game+0x1d90 (the camera's yaw), game+0x1ea0..0x1ed0 (the camera),
// uint8 blocked and typing (the gates the function checks, as it checks them), uint8 the
// mount's flags (byId(+0x1a8)+0x80, 0xff without one), pad, the three statics
// (0x551594, 12 bytes) before and after, the player before and after, uint32 buffs and
// 0x18 bytes each. CubeVulkan's src/NativeControl.cpp is checked against it.
namespace controloracle {
using updateprobe::put;
using updateprobe::putBytes;
constexpr size_t CREATURE_SIZE = updateprobe::CREATURE_SIZE;
constexpr uintptr_t CONTROL_RVA = 0x9c050, STATICS_RVA = 0x551594;
using Control = void(__fastcall *)(uint8_t *game, double dt);
static Control original = nullptr;
static std::atomic<int> remaining{0};
static std::mutex lock;
static FILE *out = nullptr;

static std::atomic<uint8_t *> localPlayer{nullptr};

static void hook(uint8_t *game, double dt) {
    uint8_t *world = *(uint8_t **)(game + 8);
    uint8_t *player = world ? *(uint8_t **)(world + 0x448) : nullptr;
    localPlayer = player;
    if (remaining.load() <= 0) {
        original(game, dt);
        return;
    }
    if (!player) {
        original(game, dt);
        return;
    }
    const uintptr_t b = base();
    uint8_t blocked = ((void *(*)(void *))(b + 0x342090))(*(void **)(game + 0x20)) != nullptr;
    blocked = blocked || (*(uint8_t **)(game + 0x398))[0x1d8] != 0;
    if (!blocked) {
        uint8_t *a = *(uint8_t **)(*(uint8_t **)(game + 0x310) + 0x68);
        blocked = (*(int32_t **)(a + 0xe0))[*(int32_t *)(a + 0xac)] != 0;
    }
    blocked = blocked || (game[0x668] && game[0x1958]);
    const uint8_t typing = ((bool (*)(void *))(b + 0x1014f0))(game + 0x118);
    uint8_t mount = 0xff;
    if (player[0x68] == 0x80)
        if (uint8_t *m = ((uint8_t * (*)(void *, int64_t))(b + 0xe720))(world, *(int64_t *)(player + 0x1a8)))
            mount = m[0x80];
    std::vector<uint8_t> r;
    put(r, dt);
    putBytes(r, game + 0x1a70, 0x10);
    putBytes(r, game + 0x1d6c, 8);
    putBytes(r, game + 0x1d90, 8);
    putBytes(r, game + 0x1ea0, 0x30);
    put(r, blocked), put(r, typing), put(r, mount), put(r, (uint8_t)0);
    putBytes(r, (void *)(b + STATICS_RVA), 12);
    std::vector<uint8_t> before;
    putBytes(before, player, CREATURE_SIZE);
    original(game, dt);
    putBytes(r, (void *)(b + STATICS_RVA), 12);
    r.insert(r.end(), before.begin(), before.end());
    putBytes(r, player, CREATURE_SIZE);
    const auto buffs = updateprobe::listOf(*(void **)(player + 0x980));
    put(r, (uint32_t)buffs.size());
    for (uint8_t *n : buffs)
        putBytes(r, n + 0x10, 0x18);
    std::lock_guard<std::mutex> hold(lock);
    if (!out)
        return;
    const uint32_t size = (uint32_t)r.size();
    fwrite(&size, 4, 1, out);
    fwrite(r.data(), 1, r.size(), out);
    if (--remaining <= 0) {
        fclose(out);
        out = nullptr;
        extract::log("Controller oracle: done");
    }
}

static void poll() {
    if (GetFileAttributesA("Extract\\control.request") == INVALID_FILE_ATTRIBUTES || !original)
        return;
    int n = 600;
    if (FILE *f = nullptr; fopen_s(&f, "Extract\\control.request", "r") == 0 && f) {
        if (fscanf_s(f, "%d", &n) != 1)
            n = 600;
        fclose(f);
    }
    DeleteFileA("Extract\\control.request");
    std::lock_guard<std::mutex> hold(lock);
    if (out)
        return;
    CreateDirectoryA("Extract\\oracle", nullptr);
    if (fopen_s(&out, "Extract\\oracle\\control.bin", "wb") != 0 || !out) {
        out = nullptr;
        return;
    }
    fwrite("CCT1", 1, 4, out);
    remaining = n;
    extract::log("Controller oracle: recording " + std::to_string(n) + " calls");
}

// Attack starts (Extract\attack.request "<count>"): the local player's calls of 0x60460
// (the next hit) and 0x60720 (charging) go to Extract\oracle\attack.bin: "CAT1", then per
// call: uint32 size, uint8 which (0 0x60460, 1 0x60720), uint8 the class key flag, uint8
// what it returned, pad, uint32 buffs and 0x18 bytes each, the player before and after.
using Start = bool(__fastcall *)(uint8_t *creature, uint8_t force);
static Start originalHit = nullptr, originalCharge = nullptr;
static std::atomic<int> attacksLeft{0};
static FILE *attackOut = nullptr;

static bool recordStart(int which, Start f, uint8_t *c, uint8_t force) {
    if (attacksLeft.load() <= 0 || c != localPlayer.load())
        return f(c, force);
    std::vector<uint8_t> r;
    std::vector<uint8_t> before;
    putBytes(before, c, CREATURE_SIZE);
    const auto buffs = updateprobe::listOf(*(void **)(c + 0x980));
    std::vector<uint8_t> buffBytes;
    for (uint8_t *n : buffs)
        putBytes(buffBytes, n + 0x10, 0x18);
    const bool result = f(c, force);
    put(r, (uint8_t)which), put(r, force), put(r, (uint8_t)result), put(r, (uint8_t)0);
    put(r, (uint32_t)buffs.size());
    r.insert(r.end(), buffBytes.begin(), buffBytes.end());
    r.insert(r.end(), before.begin(), before.end());
    putBytes(r, c, CREATURE_SIZE);
    std::lock_guard<std::mutex> hold(lock);
    if (!attackOut)
        return result;
    const uint32_t size = (uint32_t)r.size();
    fwrite(&size, 4, 1, attackOut);
    fwrite(r.data(), 1, r.size(), attackOut);
    if (--attacksLeft <= 0) {
        fclose(attackOut);
        attackOut = nullptr;
        extract::log("Attack oracle: done");
    }
    return result;
}
static bool hitHook(uint8_t *c, uint8_t force) { return recordStart(0, originalHit, c, force); }
static bool chargeHook(uint8_t *c, uint8_t force) { return recordStart(1, originalCharge, c, force); }

static void pollAttacks() {
    if (GetFileAttributesA("Extract\\attack.request") == INVALID_FILE_ATTRIBUTES || !originalHit || !originalCharge)
        return;
    int n = 200;
    if (FILE *f = nullptr; fopen_s(&f, "Extract\\attack.request", "r") == 0 && f) {
        if (fscanf_s(f, "%d", &n) != 1)
            n = 200;
        fclose(f);
    }
    DeleteFileA("Extract\\attack.request");
    std::lock_guard<std::mutex> hold(lock);
    if (attackOut)
        return;
    CreateDirectoryA("Extract\\oracle", nullptr);
    if (fopen_s(&attackOut, "Extract\\oracle\\attack.bin", "wb") != 0 || !attackOut) {
        attackOut = nullptr;
        return;
    }
    fwrite("CAT1", 1, 4, attackOut);
    attacksLeft = n;
    extract::log("Attack oracle: recording " + std::to_string(n) + " calls");
}

// Sends a function to `to` through a block near it; its first `n` bytes (whole
// instructions, none relative) run there first. Returns the way to the original.
static void *detour(uintptr_t rva, const uint8_t *start, size_t n, void *to) {
    auto *at = (uint8_t *)(base() + rva);
    uint8_t *blk = memcmp(at, start, n) == 0 ? chatlog::nearBlock((uintptr_t)at) : nullptr;
    if (!blk)
        return nullptr;
    auto absoluteJump = [](uint8_t *p, uintptr_t dest) {
        p[0] = 0xff, p[1] = 0x25, *(uint32_t *)(p + 2) = 0;
        *(uintptr_t *)(p + 6) = dest;
    };
    memcpy(blk, start, n);
    absoluteJump(blk + n, (uintptr_t)at + n);
    absoluteJump(blk + 32, (uintptr_t)to);
    DWORD was;
    if (!VirtualProtect(at, n, PAGE_EXECUTE_READWRITE, &was))
        return nullptr;
    at[0] = 0xe9;
    *(int32_t *)(at + 1) = (int32_t)((intptr_t)(blk + 32) - ((intptr_t)at + 5));
    for (size_t i = 5; i < n; i++)
        at[i] = 0x90;
    VirtualProtect(at, n, was, &was);
    FlushInstructionCache(GetCurrentProcess(), at, n);
    return blk;
}

static void install() {
    if (!supported())
        return;
    // mov rax, rsp; push rbx, rdi, r14; sub rsp, 0x110
    static const uint8_t control[14] = {0x48, 0x8b, 0xc4, 0x53, 0x57, 0x41, 0x56, 0x48, 0x81, 0xec, 0x10, 0x01, 0x00, 0x00};
    original = (Control)detour(CONTROL_RVA, control, sizeof control, (void *)&hook);
    if (!original)
        extract::log("Controller oracle: unexpected code at 0x9c050; not installed");
    // mov [rsp+0x10], rbx; push rdi; sub rsp, 0x20
    static const uint8_t hit[10] = {0x48, 0x89, 0x5c, 0x24, 0x10, 0x57, 0x48, 0x83, 0xec, 0x20};
    originalHit = (Start)detour(0x60460, hit, sizeof hit, (void *)&hitHook);
    // mov r11, rsp; push rdi; push r15; sub rsp, 0x88
    static const uint8_t charge[13] = {0x4c, 0x8b, 0xdc, 0x57, 0x41, 0x57, 0x48, 0x81, 0xec, 0x88, 0x00, 0x00, 0x00};
    originalCharge = (Start)detour(0x60720, charge, sizeof charge, (void *)&chargeHook);
    if (!originalHit || !originalCharge)
        extract::log("Attack oracle: unexpected code at 0x60460 or 0x60720; not installed");
}
} // namespace controloracle

// --- AI oracle (Extract\ai.request) ---------------------------------------------------
// The game's creature AI runs on the server world's own thread: 0x83f20 calls 0x13830
// about every 40 ms with the server's AI state (server+0x768: [0] the world, [1] a map of
// creature id to its AI record) and the ms since its last call. "ai.request" holds a
// count of ticks to record and how many to skip between them; they go to
// Extract\oracle\ai.bin (CAI2). A recorded tick holds the world's lock (world+0x3c8,
// which the tick takes itself and which re-enters) from before to after the call, so
// the game's other threads wait and the record is what the tick saw. Each record has
// the dt, the world's time, the loaded zones with their objects, every creature (its
// record and containers) and every AI record before and after the call, then each block
// the tick looked up (0xeb90), every rand() and timeGetTime() it took and each cell spot
// it asked a zone for (0x31c0e0). Read-only for
// the game.
namespace aiprobe {
constexpr uintptr_t TICK_RVA = 0x13830, BLOCK_RVA = 0xeb90, SPOT_RVA = 0x31c0e0;
constexpr size_t RECORD = 0xc10; // what of a creature the AI reads
using Tick = void(__fastcall *)(void *ai, uint32_t dt);
using Block = uint8_t *(__fastcall *)(void *zone, uint8_t *out, int x, int y, int z);
using Spot = int64_t *(__fastcall *)(void *zone, int64_t *out, int cellX, int cellY);
static Tick originalTick = nullptr;
static Block originalBlock = nullptr;
static Spot originalSpot = nullptr;
static std::atomic<int> remaining{0};
static int every = 1, skipped = 0;
static std::mutex lock;
static FILE *out = nullptr;
static thread_local std::vector<uint8_t> *looked = nullptr, *spots = nullptr;

// timeGetTime() as the game imports it (its IAT slot at 0x42ca18).
using Time = DWORD(WINAPI *)();
static Time realTime = nullptr;
static thread_local std::vector<uint32_t> *times = nullptr;
static DWORD WINAPI countedTime() {
    const DWORD v = realTime();
    if (times)
        times->push_back(v);
    return v;
}

using updateprobe::listOf;
using updateprobe::put;
using updateprobe::putBytes;

// A std::map or std::set's nodes in order (MSVC: left +0, parent +8, right +0x10,
// is-nil +0x19).
static std::vector<uint8_t *> treeOf(uint8_t *head) {
    std::vector<uint8_t *> v;
    if (!head)
        return v;
    uint8_t *n = *(uint8_t **)head;
    for (int guard = 0; n && n != head && guard < 1000000; guard++) {
        v.push_back(n);
        uint8_t *right = *(uint8_t **)(n + 0x10);
        if (!right[0x19]) {
            n = right;
            while (!(*(uint8_t **)n)[0x19])
                n = *(uint8_t **)n;
        } else {
            uint8_t *p = *(uint8_t **)(n + 8);
            while (!p[0x19] && n == *(uint8_t **)(p + 0x10))
                n = p, p = *(uint8_t **)(n + 8);
            n = p;
        }
    }
    return v;
}

// 0x31c0e0: a spot in a 16-block cell of a zone (from its generator data).
static int64_t *__fastcall spotHook(void *zone, int64_t *o, int cellX, int cellY) {
    int64_t *r = originalSpot(zone, o, cellX, cellY);
    if (spots) {
        put(*spots, cellX), put(*spots, cellY);
        put(*spots, r[0]), put(*spots, r[1]), put(*spots, r[2]);
    }
    return r;
}

static uint8_t *__fastcall blockHook(void *zone, uint8_t *o, int x, int y, int z) {
    uint8_t *r = originalBlock(zone, o, x, y, z);
    if (looked) {
        put(*looked, *(int32_t *)((uint8_t *)zone + 0x10)), put(*looked, *(int32_t *)((uint8_t *)zone + 0x14));
        put(*looked, x), put(*looked, y), put(*looked, z);
        looked->insert(looked->end(), r, r + 6);
    }
    return r;
}

// A creature: its record and the containers the AI reads or changes.
static void putCreature(std::vector<uint8_t> &r, uint8_t *c) {
    put(r, (uint64_t)c);
    putBytes(r, c, RECORD);
    auto putList = [&](size_t at, size_t bytes) {
        const auto nodes = listOf(*(void **)(c + at));
        put(r, (uint32_t)nodes.size());
        for (uint8_t *n : nodes)
            putBytes(r, n + 0x10, bytes);
    };
    putList(0x980, 0x18); // buffs
    putList(0xab8, 0xc8); // interactions
    for (size_t at : {(size_t)0xb88, (size_t)0xb98}) { // maps of creature id to a float
        const auto nodes = treeOf(*(uint8_t **)(c + at));
        put(r, (uint32_t)nodes.size());
        for (uint8_t *n : nodes)
            putBytes(r, n + 0x20, 0xc);
    }
    {
        const auto nodes = treeOf(*(uint8_t **)(c + 0xc08)); // its group's creature ids
        put(r, (uint32_t)nodes.size());
        for (uint8_t *n : nodes)
            putBytes(r, n + 0x20, 8);
    }
    {
        auto *begin = *(uint8_t **)(c + 0xbd0), *end = *(uint8_t **)(c + 0xbd8);
        const uint32_t n = begin && end > begin ? (uint32_t)((end - begin) / 4) : 0;
        put(r, n);
        putBytes(r, begin, (size_t)n * 4);
    }
    // The inventory (tabs of 0xa4-byte stacks) of creatures that aren't players.
    if (c[0x60] != 0) {
        auto *begin = *(uint8_t **)(c + 0x9f0), *end = *(uint8_t **)(c + 0x9f8);
        const uint32_t tabs = begin && end > begin ? (uint32_t)((end - begin) / 0x18) : 0;
        put(r, tabs);
        for (uint32_t t = 0; t < tabs; t++) {
            auto *b = *(uint8_t **)(begin + t * 0x18), *e = *(uint8_t **)(begin + t * 0x18 + 8);
            const uint32_t n = b && e > b ? (uint32_t)((e - b) / 0xa4) : 0;
            put(r, n);
            putBytes(r, b, (size_t)n * 0xa4);
        }
    } else
        put(r, (uint32_t)0);
}

// An AI record (the map node, 0x178 bytes) and its containers.
static void putEntry(std::vector<uint8_t> &r, uint8_t *n) {
    putBytes(r, n, 0x178);
    auto putList = [&](size_t at, size_t bytes) {
        const auto nodes = listOf(*(void **)(n + at));
        put(r, (uint32_t)nodes.size());
        for (uint8_t *e : nodes)
            putBytes(r, e + 0x10, bytes);
    };
    putList(0x78, 0xc8); // interactions waiting for the creature
    {
        const auto nodes = treeOf(*(uint8_t **)(n + 0xc0)); // the search's blocks: key, 6 ints
        put(r, (uint32_t)nodes.size());
        for (uint8_t *e : nodes)
            putBytes(r, e + 0x1c, 0x24);
    }
    {
        const auto nodes = treeOf(*(uint8_t **)(n + 0xd0)); // its open blocks
        put(r, (uint32_t)nodes.size());
        for (uint8_t *e : nodes)
            putBytes(r, e + 0x1c, 0xc);
    }
    {
        auto *best = *(uint8_t **)(n + 0xb8);
        const uint8_t end = !best || best == *(uint8_t **)(n + 0xc0);
        put(r, end);
        if (!end)
            putBytes(r, best + 0x1c, 0xc);
    }
    putList(0x138, 0xc); // the path
    {
        const auto nodes = treeOf(*(uint8_t **)(n + 0x148)); // objects it has taken
        put(r, (uint32_t)nodes.size());
        for (uint8_t *e : nodes)
            putBytes(r, e + 0x20, 8);
    }
    putList(0x158, 0x10);
}

static void putState(std::vector<uint8_t> &r, uint8_t *ai) {
    uint8_t *w = *(uint8_t **)ai;
    {
        std::vector<uint8_t *> list;
        for (uint8_t *n : listOf(*(void **)(w + 0x40)))
            list.push_back(*(uint8_t **)(n + 0x10));
        put(r, (uint32_t)list.size());
        for (uint8_t *c : list)
            putCreature(r, c);
    }
    {
        const auto nodes = treeOf(*(uint8_t **)(w + 0x450)); // id to creature
        put(r, (uint32_t)nodes.size());
        for (uint8_t *n : nodes)
            putBytes(r, n + 0x20, 0x10);
    }
    {
        const auto nodes = treeOf(*(uint8_t **)(ai + 8));
        put(r, (uint32_t)nodes.size());
        for (uint8_t *n : nodes)
            putEntry(r, n);
    }
    // What the tick tells the world: aggro (0x140), buff events (0x150), calls (0x170).
    for (auto [at, bytes] : {std::pair<size_t, size_t>{0x140, 0x18}, {0x150, 0x30}, {0x170, 8}}) {
        const auto nodes = listOf(*(void **)(w + at));
        put(r, (uint32_t)nodes.size());
        for (uint8_t *n : nodes)
            putBytes(r, n + 0x10, bytes);
    }
}

static void putZones(std::vector<uint8_t> &r, uint8_t *w) {
    const auto nodes = listOf(*(void **)(w + 0x468));
    put(r, (uint32_t)nodes.size());
    for (uint8_t *n : nodes) {
        putBytes(r, n + 0x10, 8);
        uint8_t *zone = *(uint8_t **)(n + 0x18);
        put(r, (uint64_t)zone);
        if (!zone)
            continue;
        auto *begin = *(uint8_t **)(zone + 0x60), *end = *(uint8_t **)(zone + 0x68);
        const uint32_t count = begin && end > begin ? (uint32_t)((end - begin) / 0xe0) : 0;
        put(r, (uint64_t)begin), put(r, count);
        putBytes(r, begin, (size_t)count * 0xe0);
    }
}

static void tick(void *ai, uint32_t dt) {
    if (remaining.load() <= 0 || !ai) {
        originalTick(ai, dt);
        return;
    }
    if (skipped++ < every - 1) {
        originalTick(ai, dt);
        return;
    }
    skipped = 0;
    auto *state = (uint8_t *)ai;
    uint8_t *w = *(uint8_t **)state;
    // Both of the world's locks (0x3c8 the creatures, 0x3f0 what the search takes); a
    // tick whose second lock is busy runs unrecorded, so no lock is ever waited for in
    // another order than the game's own.
    auto *section = (LPCRITICAL_SECTION)(w + 0x3c8), *search = (LPCRITICAL_SECTION)(w + 0x3f0);
    EnterCriticalSection(section);
    if (!TryEnterCriticalSection(search)) {
        LeaveCriticalSection(section);
        skipped = every;
        originalTick(ai, dt);
        return;
    }
    std::vector<uint8_t> r;
    put(r, dt);
    float time = 0;
    animprobe::readSafe(w + 0x440, &time, 4);
    put(r, time);
    float day = 0; // the time of day (ms)
    animprobe::readSafe(w + 0xc, &day, 4);
    put(r, day);
    put(r, *(uint64_t *)(w + 0x448));
    putZones(r, w);
    putState(r, state);
    std::vector<uint8_t> blocks, cellSpots;
    std::vector<int32_t> drawn;
    std::vector<uint32_t> clock;
    looked = &blocks, spots = &cellSpots, updateprobe::draws = &drawn, times = &clock;
    originalTick(ai, dt);
    looked = nullptr, spots = nullptr, updateprobe::draws = nullptr, times = nullptr;
    putState(r, state);
    LeaveCriticalSection(search);
    LeaveCriticalSection(section);
    put(r, (uint32_t)(blocks.size() / 26));
    r.insert(r.end(), blocks.begin(), blocks.end());
    put(r, (uint32_t)drawn.size());
    for (int32_t v : drawn)
        put(r, v);
    put(r, (uint32_t)clock.size());
    for (uint32_t v : clock)
        put(r, v);
    put(r, (uint32_t)(cellSpots.size() / 32));
    r.insert(r.end(), cellSpots.begin(), cellSpots.end());
    std::lock_guard<std::mutex> hold(lock);
    if (out) {
        const uint32_t n = (uint32_t)r.size();
        fwrite(&n, 4, 1, out);
        fwrite(r.data(), 1, r.size(), out);
        if (remaining.fetch_sub(1) <= 1) {
            fclose(out);
            out = nullptr;
            extract::log("AI oracle: done");
        }
    }
}

// Called each tick: starts a recording when Extract\ai.request appears.
static void poll(cube::World *w) {
    if (GetFileAttributesA("Extract\\ai.request") == INVALID_FILE_ATTRIBUTES || !originalTick)
        return;
    int n = 100, gap = 1;
    if (FILE *f = nullptr; fopen_s(&f, "Extract\\ai.request", "r") == 0 && f) {
        if (fscanf_s(f, "%d %d", &n, &gap) < 1)
            n = 100;
        fclose(f);
    }
    DeleteFileA("Extract\\ai.request");
    std::lock_guard<std::mutex> hold(lock);
    if (out)
        return;
    CreateDirectoryA("Extract\\oracle", nullptr);
    if (fopen_s(&out, "Extract\\oracle\\ai.bin", "wb") != 0 || !out) {
        out = nullptr;
        return;
    }
    fwrite("CAI2", 1, 4, out);
    const int32_t seed = w ? w->seed : 0;
    fwrite(&seed, 4, 1, out);
    every = gap < 1 ? 1 : gap, skipped = 0;
    remaining = n;
    extract::log("AI oracle: recording " + std::to_string(n) + " AI ticks, one in " + std::to_string(every));
}

// Copies `size` bytes of `entry` (whole instructions, none of them relative) to a
// trampoline in `block` and points `entry` at `to`; returns the trampoline.
static void *detour(uint8_t *entry, size_t size, uint8_t *block, void *to) {
    auto absoluteJump = [](uint8_t *at, uintptr_t target) {
        at[0] = 0xff, at[1] = 0x25, *(uint32_t *)(at + 2) = 0;
        *(uintptr_t *)(at + 6) = target;
    };
    uint8_t *tramp = block, *stub = block + 32;
    memcpy(tramp, entry, size);
    absoluteJump(tramp + size, (uintptr_t)entry + size);
    absoluteJump(stub, (uintptr_t)to);
    DWORD old;
    if (!VirtualProtect(entry, size, PAGE_EXECUTE_READWRITE, &old))
        return nullptr;
    entry[0] = 0xe9;
    *(int32_t *)(entry + 1) = (int32_t)((intptr_t)stub - ((intptr_t)entry + 5));
    for (size_t i = 5; i < size; i++)
        entry[i] = 0x90;
    VirtualProtect(entry, size, old, &old);
    FlushInstructionCache(GetCurrentProcess(), entry, size);
    return tramp;
}

static void install() {
    if (!supported())
        return;
    auto *tickAt = (uint8_t *)(base() + TICK_RVA), *blockAt = (uint8_t *)(base() + BLOCK_RVA),
         *spotAt = (uint8_t *)(base() + SPOT_RVA);
    // mov rax, rsp; push rbp, r12, r13, r14, r15 (12 bytes); push rbx; sub rsp, 0x40 (6); and
    // three movs of rbx, rbp, rsi to the caller's space (15).
    static const uint8_t tickStart[12] = {0x48, 0x8b, 0xc4, 0x55, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57};
    static const uint8_t blockStart[6] = {0x40, 0x53, 0x48, 0x83, 0xec, 0x40};
    static const uint8_t spotStart[15] = {0x48, 0x89, 0x5c, 0x24, 0x08, 0x48, 0x89, 0x6c, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18};
    if (memcmp(tickAt, tickStart, sizeof tickStart) != 0 || memcmp(blockAt, blockStart, sizeof blockStart) != 0 ||
        memcmp(spotAt, spotStart, sizeof spotStart) != 0) {
        extract::log("AI oracle: unexpected code at 0x13830, 0xeb90 or 0x31c0e0; not installed");
        return;
    }
    uint8_t *tickBlock = chatlog::nearBlock((uintptr_t)tickAt);
    uint8_t *blockBlock = tickBlock ? chatlog::nearBlock((uintptr_t)blockAt) : nullptr;
    uint8_t *spotBlock = blockBlock ? chatlog::nearBlock((uintptr_t)spotAt) : nullptr;
    if (!spotBlock)
        return;
    {
        auto *slot = (Time *)(base() + 0x42ca18);
        DWORD was;
        if (VirtualProtect(slot, sizeof *slot, PAGE_READWRITE, &was)) {
            realTime = *slot;
            *slot = &countedTime;
            VirtualProtect(slot, sizeof *slot, was, &was);
        }
    }
    originalBlock = (Block)detour(blockAt, sizeof blockStart, blockBlock, (void *)&blockHook);
    originalSpot = (Spot)detour(spotAt, sizeof spotStart, spotBlock, (void *)&spotHook);
    originalTick = (Tick)detour(tickAt, sizeof tickStart, tickBlock, (void *)&tick);
}
} // namespace aiprobe

// --- Write watch (Extract\watch.request) ----------------------------------------------
// Finds the code that writes a creature field: "watch.request" holds a field offset (hex)
// and seconds. The nearest creature that is not the player gets a hardware write
// breakpoint (DR0) on that field of its record; every instruction that writes it is
// counted for the time given, then Extract\watch.txt lists them (RVA after the write,
// count). Read-only for the game: the handler only counts and continues.
namespace writewatch {
static std::mutex lock;
static std::map<uintptr_t, int> hits;
static std::atomic<bool> active{false};
static PVOID handler = nullptr;
static DWORD gameThread = 0;
static uint64_t endTick = 0;
static uintptr_t watched = 0;
static int watchedOffset = 0;

static LONG CALLBACK onException(EXCEPTION_POINTERS *e) {
    if (e->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP || !(e->ContextRecord->Dr6 & 1))
        return EXCEPTION_CONTINUE_SEARCH;
    {
        std::lock_guard<std::mutex> hold(lock);
        hits[(uintptr_t)e->ExceptionRecord->ExceptionAddress]++;
    }
    e->ContextRecord->Dr6 = 0;
    return EXCEPTION_CONTINUE_EXECUTION;
}

// Sets (or clears) DR0 on every thread of the game from a helper thread: the code that
// writes a field may run on any of them (the server's world has its own).
static void setBreakpoint(uintptr_t address) {
    std::thread([address] {
        const DWORD helper = GetCurrentThreadId(), pid = GetCurrentProcessId();
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snap == INVALID_HANDLE_VALUE)
            return;
        THREADENTRY32 te{};
        te.dwSize = sizeof te;
        for (BOOL ok = Thread32First(snap, &te); ok; ok = Thread32Next(snap, &te)) {
            if (te.th32OwnerProcessID != pid || te.th32ThreadID == helper)
                continue;
            HANDLE t = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
            if (!t)
                continue;
            SuspendThread(t);
            CONTEXT c{};
            c.ContextFlags = CONTEXT_DEBUG_REGISTERS;
            if (GetThreadContext(t, &c)) {
                c.Dr0 = address;
                // L0, a write breakpoint (RW0 = 01), 4 bytes (LEN0 = 11).
                c.Dr7 = address ? (c.Dr7 & ~0xf0003ull) | 1 | (1ull << 16) | (3ull << 18) : (c.Dr7 & ~0xf0003ull);
                SetThreadContext(t, &c);
            }
            ResumeThread(t);
            CloseHandle(t);
        }
        CloseHandle(snap);
    }).join();
}

static void poll(cube::World *w) {
    if (active.load()) {
        if (GetTickCount64() < endTick)
            return;
        setBreakpoint(0);
        active = false;
        FILE *f = nullptr;
        if (fopen_s(&f, "Extract\\watch.txt", "w") == 0 && f) {
            std::lock_guard<std::mutex> hold(lock);
            fprintf(f, "# writes to creature+0x%x of %p: RVA after the writing instruction, count\n", watchedOffset, (void *)watched);
            for (auto &[at, n] : hits)
                fprintf(f, "0x%llx %d\n", (unsigned long long)(at - base()), n);
            fclose(f);
        }
        extract::log("Write watch: done, " + std::to_string(hits.size()) + " writers");
        return;
    }
    if (GetFileAttributesA("Extract\\watch.request") == INVALID_FILE_ATTRIBUTES || !w || !w->local_creature)
        return;
    int offset = 0x40, seconds = 10, server = 0, player = 0; // fourth number 1: the local player itself
    if (FILE *f = nullptr; fopen_s(&f, "Extract\\watch.request", "r") == 0 && f) {
        if (fscanf_s(f, "%x %d %d %d", &offset, &seconds, &server, &player) < 1)
            offset = 0x40;
        fclose(f);
    }
    DeleteFileA("Extract\\watch.request");
    auto *self = (uint8_t *)w->local_creature;
    const int64_t px = *(int64_t *)(self + 0x10), py = *(int64_t *)(self + 0x18);
    uint8_t *best = nullptr;
    double bestD = 1e300;
    // In the server's world (third number 1) the creatures are its own copies, run on its thread.
    uint8_t *inWorld = (uint8_t *)w;
    gameThread = GetCurrentThreadId();
    if (server) {
        inWorld = updateprobe::serverWorld.load();
        gameThread = updateprobe::serverThread.load();
        if (!inWorld || !gameThread)
            return;
    }
    uint8_t *head = *(uint8_t **)(inWorld + 0x40);
    int guard = 0;
    for (uint8_t *n = *(uint8_t **)head; n && n != head && guard < 100000; n = *(uint8_t **)n, guard++) {
        uint8_t *c = *(uint8_t **)(n + 0x10);
        if (c == self || *(int64_t *)(c + 8) == *(int64_t *)(self + 8) || c[0x60] == 0) // players
            continue;
        const double dx = (double)(*(int64_t *)(c + 0x10) - px), dy = (double)(*(int64_t *)(c + 0x18) - py);
        if (dx * dx + dy * dy < bestD)
            bestD = dx * dx + dy * dy, best = c;
    }
    if (player && !server)
        best = self, bestD = 0;
    if (!best)
        return;
    {
        std::lock_guard<std::mutex> hold(lock);
        hits.clear();
    }
    if (!handler)
        handler = AddVectoredExceptionHandler(1, onException);
    watched = (uintptr_t)best + offset;
    watchedOffset = offset;
    setBreakpoint(watched);
    endTick = GetTickCount64() + (uint64_t)seconds * 1000;
    active = true;
    extract::log("Write watch: creature+" + std::to_string(offset) + " (decimal), race " + std::to_string(*(int *)(best + 0x64)) +
                 ", hostility " + std::to_string(best[0x60]) + ", " + std::to_string(std::sqrt(bestD) / 65536) + " blocks away" + " of the nearest creature, " + std::to_string(seconds) + " s");
}
} // namespace writewatch

// --- Conversation scripts ------------------------------------------------------------
// K: asks the game itself what every loaded townsperson would say (RVA 0x2d9480:
// world, std::vector<std::string>* out, creature), the script its speech system then
// plays: speech ids and commands such as define_location x y type, define_item ...,
// mark_location, new_page, !emotion. One line per creature to
// Extract\talk-scripts.txt, tokens separated by " | ". The game's own vector
// destructor (RVA 0x81790) frees the result, so its allocator stays in charge.
// --- World generator oracle (O) --------------------------------------------------
// Asks the game's own terrain functions for every intermediate value of the ground
// height (0x35ea0) at a fixed set of points and writes Extract\gen-oracle.txt, so
// CubeVulkan's reimplementation (src/GameGen.cpp) can be checked value by value.
// Read-only: the functions compute from coordinates and change nothing.
namespace genoracle {
struct Nb { // 0x358e0's structure
    int biome[3][3];
    float dist2[3][3];
    int x, y;
    float nearest;
    int nearestBiome;
};
struct Pl { // 0x35ce0's structure
    int kind;
    bool big;
    int variant, radius, cx, cy;
    float dist2, weight;
};
static_assert(sizeof(Nb) == 0x58 && sizeof(Pl) == 0x20, "generator structures");
static void dump(cube::Game *g) {
    cube::World *w = g->world;
    uintptr_t b = base();
    auto nb = (void (*)(cube::World *, int, int, Nb *))(b + 0x358e0);
    auto pl = (void (*)(cube::World *, int, int, Pl *))(b + 0x35ce0);
    auto lake = (float (*)(Nb *))(b + 0x2bb7e0);
    auto baseH = (float (*)(Nb *, int, float))(b + 0x2a0840);
    auto roads = (float (*)(cube::World *, int, int, int, float))(b + 0x2d4730);
    auto w13 = (float (*)(Nb *, int, float))(b + 0x2b9750);
    auto w125 = (float (*)(Nb *, int, float))(b + 0x2d9f30);
    auto w8 = (float (*)(Nb *))(b + 0x2e64e0);
    auto river = (float (*)(cube::World *, int, int, float, float, float, float, float, Pl *))(b + 0x2e5f80);
    auto lava = (float (*)(cube::World *, int, int, float, float, float, Pl *))(b + 0x2b92f0);
    auto swamp = (float (*)(cube::World *, int, int, float, float, float, Pl *))(b + 0x2d01a0);
    auto high = (float (*)(cube::World *, int, int, float, float, float, float, float, float, Pl *))(b + 0x2bb310);
    auto a44 = (float (*)(Nb *))(b + 0x2a44a0);
    auto b59 = (float (*)(Nb *))(b + 0x2b5960);
    auto veg = (float (*)(cube::World *, int, int, float, float, float))(b + 0x2d36f0);
    auto ground = (float (*)(cube::World *, int, int, int))(b + GROUND_HEIGHT_RVA);
    auto regionAt = (void (*)(cube::World *, IntVector2 *, int, int))(b + REGION_AT_RVA);
    CreateDirectoryA("Extract", nullptr);
    FILE *f = _fsopen("Extract\\gen-oracle.txt", "w", _SH_DENYNO);
    if (!f)
        return;
    fprintf(f, "# x y | regionX regionY | kind big variant radius cx cy dist2 weight | lake base roads w13 w125 w8 river "
               "lava swamp high a44 b59 veg | height0 height4\n");
    // Points: a grid around the player, around the world's origin, and scattered far out.
    std::vector<std::pair<int, int>> points;
    auto &e = w->local_creature->entity_data;
    int px = (int)(e.position.x >> 16), py = (int)(e.position.y >> 16);
    for (int j = -10; j <= 10; j++)
        for (int i = -10; i <= 10; i++)
            points.push_back({px + i * 37, py + j * 41});
    for (int j = -10; j <= 10; j++)
        for (int i = -10; i <= 10; i++)
            points.push_back({i * 613, j * 587});
    // Far out, where the owner's seed-123 extraction is (about -94 million blocks).
    for (int j = -10; j <= 10; j++)
        for (int i = -10; i <= 10; i++)
            points.push_back({-94042880 + i * 37, -93755584 + j * 41});
    uint32_t s = 12345;
    for (int k = 0; k < 500; k++) {
        s = s * 1664525u + 1013904223u;
        int x = (int)(s >> 4) % 4000000 - 2000000;
        s = s * 1664525u + 1013904223u;
        int y = (int)(s >> 4) % 4000000 - 2000000;
        points.push_back({-94042880 + x, -93755584 + y});
    }
    for (int k = 0; k < 2000; k++) {
        s = s * 1664525u + 1013904223u;
        int x = (int)(s >> 4) % 4000000 - 2000000;
        s = s * 1664525u + 1013904223u;
        int y = (int)(s >> 4) % 4000000 - 2000000;
        points.push_back({x, y});
    }
    for (auto [x, y] : points) {
        Nb n{};
        Pl p{};
        nb(w, x, y, &n);
        pl(w, x, y, &p);
        IntVector2 r{};
        regionAt(w, &r, x, y);
        float lk = lake(&n), bh = baseH(&n, p.kind, p.weight), rd = roads(w, x, y, p.kind, p.weight);
        float v13 = w13(&n, p.kind, p.weight), v125 = w125(&n, p.kind, p.weight), v8 = w8(&n);
        float rv = river(w, x, y, rd, v13, v125, v8, lk, &p), lv = lava(w, x, y, rd, v13, lk, &p);
        float sw = swamp(w, x, y, rd, v125, lk, &p), hi = high(w, x, y, bh, lk, rv, lv, sw, rd, &p);
        float va = a44(&n), vb = b59(&n), vg = veg(w, x, y, vb, hi, va);
        fprintf(f, "%d %d | %d %d | %d %d %d %d %d %d %.9g %.9g | %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g | %.9g %.9g\n",
                x, y, r.x, r.y, p.kind, (int)p.big, p.variant, p.radius, p.cx, p.cy, p.dist2, p.weight, lk, bh, rd, v13, v125, v8,
                rv, lv, sw, hi, va, vb, vg, ground(w, x, y, 0), ground(w, x, y, 4));
    }
    fclose(f);
    extract::log("Generator oracle: " + std::to_string(points.size()) + " points written to Extract\\gen-oracle.txt");

    // Blocks: the zone generator's column inputs (0x2ff6f0) and the block properties the
    // game makes from them (0x2d8e00) at the surface and below, with the snow and river
    // helpers it uses there.
    struct Props { float r, g, b; uint8_t c, type, e, pad; };
    auto w9b = (float (*)(Nb *, int, float))(b + 0x2d9b60);
    auto wbb = (float (*)(Nb *, int, float))(b + 0x2abb60);
    auto props = (Props * (*)(cube::World *, Props *, int, int, int, float, float, float, float, float, float, float, float,
                              float, float, float, float, float, float, float, float))(b + 0x2d8e00);
    auto snowAt = (float (*)(cube::World *, int, int, float, float, float, float, float))(b + 0x2b4320);
    auto snowColour = (float *(*)(cube::World *, float *, int, int))(b + 0x2d3d90);
    auto bedColour = (float *(*)(cube::World *, float *, int, int, int))(b + 0xbd110);
    auto waterTop = (float (*)(cube::World *, int, int, float, Pl *))(b + 0x2e6420);
    FILE *k = _fsopen("Extract\\gen-blocks.txt", "w", _SH_DENYNO);
    if (!k)
        return;
    fprintf(k, "# world+0x330 %d world+0x334 %d world+0x244 %d seed %d\n", *(int *)((uint8_t *)w + 0x330),
            *(int *)((uint8_t *)w + 0x334), *(int *)((uint8_t *)w + 0x244), w->seed);
    fprintf(k, "# x y | h hx- hx+ hy- hy+ | w9b wbb | snow snowR snowG snowB | bedR bedG bedB top | per depth 0 1 3 10 40: "
               "r g b c type e\n");
    for (int j = -16; j < 16; j++)
        for (int i = -16; i < 16; i++)
            points.push_back({px + i, py + j});
    for (auto [x, y] : points) {
        Nb n{};
        Pl p{};
        nb(w, x, y, &n);
        pl(w, x, y, &p);
        float lk = lake(&n), bh = baseH(&n, p.kind, p.weight), rd = roads(w, x, y, p.kind, p.weight);
        float v13 = w13(&n, p.kind, p.weight), v125 = w125(&n, p.kind, p.weight), v8 = w8(&n);
        float rv = river(w, x, y, rd, v13, v125, v8, lk, &p), lv = lava(w, x, y, rd, v13, lk, &p);
        float sw = swamp(w, x, y, rd, v125, lk, &p), hi = high(w, x, y, bh, lk, rv, lv, sw, rd, &p);
        float va = a44(&n), vb = b59(&n), vg = veg(w, x, y, vb, hi, va);
        float fc = w9b(&n, p.kind, p.weight), f20 = wbb(&n, p.kind, p.weight);
        float h = ground(w, x, y, 0), hxm = ground(w, x - 1, y, 0), hxp = ground(w, x + 1, y, 0);
        float hym = ground(w, x, y - 1, 0), hyp = ground(w, x, y + 1, 0);
        float sn = snowAt(w, x, y, fc, hxm, hxp, hym, hyp);
        float sc[3]{}, bc[3]{};
        snowColour(w, sc, x, y);
        int fz = (int)std::floor(h);
        bedColour(w, bc, x, y, fz - 1);
        float top = waterTop(w, x, y, bh, &p);
        fprintf(k, "%d %d | %.9g %.9g %.9g %.9g %.9g | %.9g %.9g | %.9g %.9g %.9g %.9g | %.9g %.9g %.9g %.9g |", x, y, h, hxm,
                hxp, hym, hyp, fc, f20, sn, sc[0], sc[1], sc[2], bc[0], bc[1], bc[2], top);
        for (int d : {0, 1, 3, 10, 40}) {
            Props o{};
            props(w, &o, x, y, fz - d, h, hxm, hxp, hym, hyp, hi, fc, vb, vg, 0.0f, rd, v13, v125, f20, lv, sw);
            fprintf(k, " %.9g %.9g %.9g %d %d %d", o.r, o.g, o.b, o.c, o.type, o.e);
        }
        fprintf(k, "\n");
    }
    fclose(k);
    extract::log("Generator oracle: blocks written to Extract\\gen-blocks.txt");

    // Structures: the object of each 16-block cell (0x69c60: trees, rocks, buildings...)
    // over a square around the player and scattered far out.
    auto cellObject = (int (*)(cube::World *, int, int))(b + 0x69c60);
    FILE *c = _fsopen("Extract\\gen-cells.txt", "w", _SH_DENYNO);
    if (!c)
        return;
    fprintf(c, "# cx cy object\n");
    int pcx = (int)std::floor(px / 16.0), pcy = (int)std::floor(py / 16.0);
    for (int j = -100; j < 100; j++)
        for (int i = -100; i < 100; i++)
            fprintf(c, "%d %d %d\n", pcx + i, pcy + j, cellObject(w, pcx + i, pcy + j));
    uint32_t r = 777;
    for (int n = 0; n < 20000; n++) {
        r = r * 1664525u + 1013904223u;
        int cx = (int)(r >> 4) % 250000 - 125000;
        r = r * 1664525u + 1013904223u;
        int cy = (int)(r >> 4) % 250000 - 125000;
        fprintf(c, "%d %d %d\n", cx, cy, cellObject(w, cx, cy));
    }
    fclose(c);
    extract::log("Generator oracle: cell objects written to Extract\\gen-cells.txt");
}
} // namespace genoracle

namespace talkscripts {
struct GameString { // MSVC std::string as the game lays it out
    union { char buffer[16]; char *pointer; };
    size_t size, capacity;
    std::string text() const { return std::string(capacity >= 16 ? pointer : buffer, size); }
};
struct GameVector { GameString *first, *last, *end; };
struct GameWString { // MSVC std::wstring
    union { wchar_t buffer[8]; wchar_t *pointer; };
    size_t size, capacity;
};

// The game's name generator, RVA 0x611d0(out, seed, style). People: seed = the low 32
// bits of the creature id, style = race (as define_mob and the chat's speaker name).
// Places: see locationName. UTF-8.
static std::string generatedName(int seed, int style) {
    GameWString name{};
    ((GameWString * (*)(GameWString *, int, int))(base() + 0x611d0))(&name, seed, style);
    const wchar_t *text = name.capacity >= 8 ? name.pointer : name.buffer;
    int n = WideCharToMultiByte(CP_UTF8, 0, text, (int)name.size, nullptr, 0, nullptr, nullptr);
    std::string utf8(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, (int)name.size, utf8.data(), n, nullptr, nullptr);
    if (name.capacity >= 8)
        ((void (*)(void *))(base() + 0x392be8))(name.pointer); // the game's operator delete
    return utf8;
}

// The game's proper name for the location in 1024-block cell (x, y) of `type`: style
// 0x2ba810(x, y, type), seed 0x2ba7c0(x, y), as its speech system does for
// define_location (RVA 0x160d10).
static std::string locationName(int x, int y, int type) {
    int style = ((int (*)(int, int, int))(base() + 0x2ba810))(x, y, type);
    int seed = ((int (*)(int, int))(base() + 0x2ba7c0))(x, y);
    return generatedName(seed, style);
}

// onlyNew: just the townspeople not written yet today (checked every 2 s, so tours
// collect them); otherwise all loaded ones (K). The game reseeds each script with the
// world's day (world + 8), so each person is written for SCRIPT_DAYS days from today:
// the day is set for the call and put back at once, on the game's own thread.
constexpr int SCRIPT_DAYS = 64;
static std::string itemName(cube::Game *g, int category, int id, unsigned modifier, int rarity, int material);
static void exportScript(cube::Game *g, FILE *out, cube::Creature *c, int day);
static void exportAll(cube::Game *g, bool onlyNew) {
    cube::World *w = g->world;
    static std::set<std::pair<long long, int>> written;
    CreateDirectoryA("Extract", nullptr);
    FILE *out = _fsopen("Extract\\talk-scripts.txt", "a", _SH_DENYNO);
    if (!out)
        return;
    int n = 0, today = *(int *)((uint8_t *)w + 8);
    for (cube::Creature *c : w->creatures) {
        if (!c || c == w->local_creature || c->entity_data.hostility_type != 3)
            continue;
        if (!written.insert({(long long)c->id, today}).second && onlyNew)
            continue;
        if (n == 0)
            fprintf(out, "# t=%llu day=%d\n", (unsigned long long)GetTickCount64(), today);
        for (int day = today; day < today + SCRIPT_DAYS; day++)
            exportScript(g, out, c, day);
        n++;
    }
    fclose(out);
    if (n)
        extract::log("Conversation scripts: " + std::to_string(n) + " townspeople written to Extract\\talk-scripts.txt (" +
                     std::to_string(SCRIPT_DAYS) + " days each)");
}
static void exportScript(cube::Game *g, FILE *out, cube::Creature *c, int day) {
    cube::World *w = g->world;
    auto script = (void (*)(cube::World *, GameVector *, cube::Creature *))(base() + 0x2d9480);
    auto destroy = (void (*)(GameVector *))(base() + 0x81790);
    int &worldDay = *(int *)((uint8_t *)w + 8);
    const int today = worldDay;
    {
        GameVector v{};
        worldDay = day;
        script(w, &v, c);
        worldDay = today;
        auto &e = c->entity_data;
        // speaker: the name the game gives them (generator seeded with the id's low 32 bits, race as style).
        fprintf(out, "id=%lld day=%d race=%u class=%d spec=%d flags=%08x flags2=%08x speaker=%s pos=%.1f,%.1f,%.1f :",
                (long long)c->id, day, e.race, (int)e.classType, (int)e.specialization, e.flags, e.appearance.flags2,
                generatedName((int)(uint32_t)c->id, (int)e.race).c_str(), e.position.x / 65536.0,
                e.position.y / 65536.0, e.position.z / 65536.0);
        std::vector<std::string> tokens;
        for (GameString *s = v.first; s && s != v.last; s++)
            tokens.push_back(s->text());
        for (size_t i = 0; i < tokens.size(); i++) {
            fprintf(out, " %s |", tokens[i].c_str());
            // define_location x y type: add the game's name for it as "name=<proper>".
            if (tokens[i] == "define_location" && i + 3 < tokens.size()) {
                int x = atoi(tokens[i + 1].c_str()), y = atoi(tokens[i + 2].c_str()), type = atoi(tokens[i + 3].c_str());
                for (int j = 1; j <= 3; j++)
                    fprintf(out, " %s |", tokens[i + j].c_str());
                fprintf(out, " name=%s |", locationName(x, y, type).c_str());
                i += 3;
            } else if (tokens[i] == "define_item" && i + 5 < tokens.size()) {
                // define_item category id modifier rarity material: the game's name for it and
                // its dictionary key (Speech::item_identifier_map), for the grammar.
                int v[5];
                for (int j = 0; j < 5; j++)
                    v[j] = (int)strtol(tokens[i + 1 + j].c_str(), nullptr, 10), fprintf(out, " %s |", tokens[i + 1 + j].c_str());
                std::string key;
                auto found = g->speech.item_identifier_map.find({(uint32_t)v[0], (uint32_t)v[1]});
                if (found != g->speech.item_identifier_map.end())
                    for (wchar_t ch : found->second) key += (char)ch;
                fprintf(out, " item=%s | key=%s |", itemName(g, v[0], v[1], (unsigned)v[2], v[3], v[4]).c_str(), key.c_str());
                i += 5;
            } else if (tokens[i] == "define_mob" && i + 2 < tokens.size()) { // define_mob id race: its name too
                for (int j = 1; j <= 2; j++)
                    fprintf(out, " %s |", tokens[i + j].c_str());
                fprintf(out, " name=%s |",
                        generatedName((int)strtoul(tokens[i + 1].c_str(), nullptr, 10), atoi(tokens[i + 2].c_str())).c_str());
                i += 2;
            }
        }
        fprintf(out, "\n");
        destroy(&v);
    }
}
// Speech::GenerateItemName (RVA 0x1628b0: speech, std::wstring* out, Item*), the name the
// game shows, e.g. "Hang Glider" or a weapon with its quality word.
static std::string itemName(cube::Game *g, int category, int id, unsigned modifier, int rarity, int material) {
    alignas(8) uint8_t raw[sizeof(cube::Item)] = {}; // an Item with no spirits
    auto &item = *(cube::Item *)raw;
    item.category = (char)category, item.id = id;
    item.modifier = modifier, item.rarity = (char)rarity, item.material = (char)material;
    GameWString name{};
    ((GameWString * (*)(cube::Speech *, GameWString *, cube::Item *))(base() + 0x1628b0))(&g->speech, &name, &item);
    const wchar_t *text = name.capacity >= 8 ? name.pointer : name.buffer;
    int n = WideCharToMultiByte(CP_UTF8, 0, text, (int)name.size, nullptr, 0, nullptr, nullptr);
    std::string utf8(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, (int)name.size, utf8.data(), n, nullptr, nullptr);
    if (name.capacity >= 8)
        ((void (*)(void *))(base() + 0x392be8))(name.pointer);
    return utf8;
}
} // namespace talkscripts

// --- AI and effect trace ------------------------------------------------------------
// 0 toggles it. Every 100 ms, each creature within 120 blocks of the player (and the
// player) is written to Extract\ai-trace.txt: hostility, level, position, velocity,
// ability and its time, health, combo, the unnamed words at +0x9a8..+0x9d0 (the
// creature's target is among them), and its effects (type:value:time left). Read-only.
namespace aitrace {
static bool active = false;
static FILE *log = nullptr;
static ULONGLONG next = 0;

static void toggle() {
    active = !active;
    if (active) {
        CreateDirectoryA("Extract", nullptr);
        log = _fsopen("Extract\\ai-trace.txt", "a", _SH_DENYNO);
        extract::log("AI trace on");
    } else {
        if (log)
            fclose(log);
        log = nullptr;
        extract::log("AI trace off");
    }
}

static void tick(cube::Game *g) {
    if (!active || !log || GetTickCount64() < next)
        return;
    next = GetTickCount64() + 100;
    cube::World *w = g->world;
    cube::Creature *player = w->local_creature;
    if (!player)
        return;
    auto blocks = [](int64_t v) { return double(v) / 65536; };
    fprintf(log, "t=%llu\n", (unsigned long long)GetTickCount64());
    for (cube::Creature *c : w->creatures) {
        if (!c)
            continue;
        auto &e = c->entity_data;
        double dx = blocks(e.position.x - player->entity_data.position.x),
               dy = blocks(e.position.y - player->entity_data.position.y),
               dz = blocks(e.position.z - player->entity_data.position.z);
        double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (c != player && distance > 120)
            continue;
        auto *raw = (uint8_t *)c;
        fprintf(log,
                " id=%lld race=%u host=%d lvl=%u d=%.2f pos=%.2f,%.2f,%.2f vel=%.2f,%.2f,%.2f yaw=%.1f ab=%d since=%.0f "
                "hp=%.1f combo=%d sincehit=%.0f stun=%.0f flags=%08x w=%llx,%llx,%llx,%llx,%llx,%llx fx=",
                (long long)c->id, e.race, (int)e.hostility_type, e.level, distance, blocks(e.position.x),
                blocks(e.position.y), blocks(e.position.z), e.velocity.x, e.velocity.y, e.velocity.z, e.yaw,
                e.current_ability, e.time_since_ability, e.HP, *(int *)(raw + 0x70), e.time_since_hit, e.stun_time,
                e.flags, *(unsigned long long *)(raw + 0x9a8), *(unsigned long long *)(raw + 0x9b0),
                *(unsigned long long *)(raw + 0x9b8), *(unsigned long long *)(raw + 0x9c0),
                *(unsigned long long *)(raw + 0x9c8), *(unsigned long long *)(raw + 0x9d0));
        for (auto &b : c->buffs)
            fprintf(log, "%d:%.3f:%.0f,", (int)b.type, b.unk, b.time_remaining);
        fprintf(log, "%s\n", c == player ? " PLAYER" : "");
    }
    fflush(log);
}

// = (with the trace on): an approach test on the nearest living monster. The player is
// put 50 blocks from it; every 1.5 s, 1 block closer. If the monster closes in by more
// than 2 blocks during a step the player stays put (up to 6 s) to see whether it attacks;
// an attack ("engaged") is followed by 12 s of fighting, then the player moves 2 blocks
// further every 1.5 s up to 160, so its give-up distance shows in the trace.
static int phase = 0; // 0 off, 1 closing in, 2 waiting (it approached), 3 engaged, 4 backing off
static int64_t subject = 0;
static double gap = 50;
static ULONGLONG phaseStart = 0, nextStep = 0;
static void startApproach(cube::World *w) {
    cube::Creature *player = w->local_creature, *best = nullptr;
    auto spread = [&](cube::Creature *c) {
        double dx = double(c->entity_data.position.x - player->entity_data.position.x),
               dy = double(c->entity_data.position.y - player->entity_data.position.y);
        return dx * dx + dy * dy;
    };
    for (cube::Creature *c : w->creatures)
        if (c && c != player && c->entity_data.hostility_type == 1 && c->entity_data.HP > 0 && (!best || spread(c) < spread(best)))
            best = c;
    if (!best || !log)
        return;
    subject = best->id, gap = 50, phase = 1, phaseStart = nextStep = GetTickCount64();
    fprintf(log, "approach start id=%lld race=%u\n", (long long)subject, best->entity_data.race);
}
static void approach(cube::World *w) {
    if (!phase)
        return;
    cube::Creature *player = w->local_creature, *m = nullptr;
    for (cube::Creature *c : w->creatures)
        if (c && c->id == subject)
            m = c;
    if (!m || m->entity_data.HP <= 0) {
        fprintf(log, "approach end (subject gone)\n");
        phase = 0;
        return;
    }
    auto &me = m->entity_data, &pe = player->entity_data;
    double dx = double(pe.position.x - me.position.x) / 65536, dy = double(pe.position.y - me.position.y) / 65536;
    double real = std::max(1e-6, std::sqrt(dx * dx + dy * dy));
    ULONGLONG now = GetTickCount64();
    if ((phase == 1 || phase == 2) && me.current_ability != 0) {
        fprintf(log, "approach engaged gap=%.1f real=%.2f ability=%d\n", gap, real, me.current_ability);
        phase = 3, phaseStart = now;
    }
    if (now < nextStep)
        return;
    nextStep = now + 1500;
    bool place = false;
    if (phase == 1) {
        if (real < gap - 2) {
            phase = 2, phaseStart = now;
            fprintf(log, "approach closing gap=%.1f real=%.2f\n", gap, real);
        } else
            gap -= 1, place = gap >= 1;
    } else if (phase == 2 && now - phaseStart > 6000) {
        fprintf(log, "approach no attack gap=%.1f real=%.2f\n", gap, real);
        phase = 1, place = true;
    } else if (phase == 3 && now - phaseStart > 12000)
        phase = 4, gap = real;
    else if (phase == 4) {
        gap += 2, place = true;
        if (gap > 160) {
            fprintf(log, "approach end\n");
            phase = 0;
            return;
        }
    }
    if (place) { // `gap` blocks from it, on the side the player is now
        pe.position.x = me.position.x + (int64_t)(dx / real * gap * 65536);
        pe.position.y = me.position.y + (int64_t)(dy / real * gap * 65536);
        pe.position.z = me.position.z + (2 << 16);
        pe.velocity = FloatVector3(0, 0, 0);
    }
    fprintf(log, "approach phase=%d gap=%.1f real=%.2f ability=%d vel=%.2f,%.2f\n", phase, gap, real, me.current_ability,
            me.velocity.x, me.velocity.y);
}
// - (minus): stands the player 2.5 blocks from the nearest townsperson (hostility 3)
// not visited yet this session, facing them, and logs who it is, so they can be
// spoken to (E) to see what the game does with quests.
static void visitTownsperson(cube::Game *g) {
    static std::vector<int64_t> visited;
    cube::World *w = g->world;
    cube::Creature *player = w->local_creature, *best = nullptr;
    auto spread = [&](cube::Creature *c) {
        double dx = double(c->entity_data.position.x - player->entity_data.position.x),
               dy = double(c->entity_data.position.y - player->entity_data.position.y);
        return dx * dx + dy * dy;
    };
    for (cube::Creature *c : w->creatures)
        if (c && c != player && c->entity_data.hostility_type == 3 && c->entity_data.HP > 0 &&
            std::find(visited.begin(), visited.end(), c->id) == visited.end() && (!best || spread(c) < spread(best)))
            best = c;
    if (!best)
        return;
    visited.push_back(best->id);
    auto &be = best->entity_data, &pe = player->entity_data;
    double yaw = be.yaw * 3.14159265358979 / 180;
    pe.position.x = be.position.x + (int64_t)(std::cos(yaw) * 2.5 * 65536);
    pe.position.y = be.position.y + (int64_t)(std::sin(yaw) * 2.5 * 65536);
    pe.position.z = be.position.z + (1 << 16);
    pe.velocity = FloatVector3(0, 0, 0);
    g->camera_angle.z = g->target_camera_angle.z = be.yaw + 180 + 90;
    if (log)
        fprintf(log, "visit townsperson id=%lld race=%u level=%u name=%s\n", (long long)best->id, be.race, be.level,
                be.name);
    extract::log("Visiting townsperson race " + std::to_string(be.race));
}
} // namespace aitrace

namespace character {
constexpr int SPRITES = 14, MATRICES = 33;
static const char *spriteNames[SPRITES] = {"face",   "hair",         "chest",       "unk",
                                           "hands",  "wings",        "tail",        "feet",
                                           "weapon_right", "weapon_left", "shoulder", "neck",
                                           "?", "?"};
static bool recording = false;
static FILE *file = nullptr;
static uint32_t recorded = 0;

static std::unordered_map<void *, int> slotMap(cube::World *w) {
    std::unordered_map<void *, int> slotOf;
    if (w->spritemanager)
        for (size_t i = 0; i < w->spritemanager->sprites.size(); i++)
            slotOf[w->spritemanager->sprites[i]] = (int)i;
    return slotOf;
}
static int64_t *spritePointers(cube::Creature *c) { return &c->animation_state.face_sprite; }
static const Matrix4 *matrices(cube::Creature *c) { return &c->animation_state.body_with_head_matrix; }

static std::string nameOf(void *sprite) {
    std::lock_guard lock(namesMutex);
    for (auto &n : extract::spriteNames)
        if (n.first == sprite)
            return n.second;
    return "";
}

static void snapshot(cube::World *w, cube::Creature *c) {
    std::string folder = characterFolder(c);
    FILE *f = nullptr;
    if (fopen_s(&f, (folder + "\\snapshot.txt").c_str(), "w") != 0 || !f)
        return;
    auto &e = c->entity_data;
    auto &a = e.appearance;
    fprintf(f, "name=%.16s\nrace=%u\nclass=%u\nspecialization=%d\nlevel=%u\n", e.name, e.race, e.classType,
            e.specialization, e.level);
    fprintf(f, "stats %s\n", statsText(c).c_str());
    fprintf(f, "position=%.4f %.4f %.4f\nyaw=%.4f pitch=%.4f roll=%.4f\nvelocity=%.3f %.3f %.3f\n",
            e.position.x / 65536.0, e.position.y / 65536.0, e.position.z / 65536.0, e.yaw, e.pitch, e.roll,
            e.velocity.x, e.velocity.y, e.velocity.z);
    fprintf(f, "flags=%08x ability=%d time_since_ability=%.3f\n", e.flags, e.current_ability, e.time_since_ability);
    fprintf(f, "hair_color=%d %d %d %d\nflags2=%08x\n", a.hair_color.red, a.hair_color.green, a.hair_color.blue,
            a.hair_color.alpha, a.flags2);
    fprintf(f, "scales graphics=%.4f hitbox=%.4f physics=%.4f\n", a.graphics_scale, a.hitbox_scale, a.physics_scale);
    fprintf(f, "models head=%d hair=%d hands=%d feet=%d chest=%d tail=%d shoulder=%d wings=%d\n", a.head_model,
            a.hair_model, a.hands_model, a.feet_model, a.chest_model, a.tail_model, a.shoulder_model, a.wings_model);
    fprintf(f, "part_scales head=%.4f chest=%.4f hands=%.4f feet=%.4f unknown=%.4f weapon=%.4f tail=%.4f shoulder=%.4f wings=%.4f\n",
            a.head_scale, a.chest_scale, a.hands_scale, a.feet_scale, a.unknown_scale, a.weapon_scale, a.tail_scale,
            a.shoulder_scale, a.wing_scale);
    auto v3 = [&](const char *n, const FloatVector3 &v) { fprintf(f, "%s=%.4f %.4f %.4f\n", n, v.x, v.y, v.z); };
    fprintf(f, "rotations chest=%.4f feet=%.4f wings=%.4f unknown=%.4f\n", a.chest_rotation, a.feet_rotation,
            a.wings_rotation, a.unknown_rotation);
    v3("hands_rotation", a.hands_rotation);
    v3("chest_position", a.chest_position);
    v3("head_position", a.head_position);
    v3("hands_position", a.hands_position);
    v3("feet_position", a.feet_position);
    v3("unknown_position", a.unknown_position);
    v3("wings_position", a.wings_position);
    const cube::Item *items = &e.equipment.unk_item;
    static const char *slots[] = {"unk", "neck", "chest", "feet", "hands", "shoulder", "weapon_left",
                                  "weapon_right", "ring_left", "ring_right", "pet"};
    for (int i = 0; i < 11; i++)
        fprintf(f, "equip %s category=%d id=%d material=%d rarity=%d modifier=%u\n", slots[i], items[i].category,
                items[i].id, items[i].material, items[i].rarity, items[i].modifier);
    auto &s = c->animation_state;
    fprintf(f, "anim state=%d timer=%d\n", s.current_animation_state_id, s.current_animation_state_timer);
    auto slotOf = slotMap(w);
    int64_t *sprites = spritePointers(c);
    for (int i = 0; i < SPRITES; i++) {
        void *p = (void *)sprites[i];
        auto it = slotOf.find(p);
        fprintf(f, "sprite %s slot=%d %s\n", spriteNames[i], it == slotOf.end() ? -1 : it->second,
                p ? nameOf(p).c_str() : "");
    }
    const Matrix4 *m = matrices(c);
    for (int i = 0; i < MATRICES; i++)
        fprintf(f, "matrix %d: %.4f %.4f %.4f %.4f | %.4f %.4f %.4f %.4f | %.4f %.4f %.4f %.4f | %.4f %.4f %.4f %.4f\n", i,
                m[i]._11, m[i]._12, m[i]._13, m[i]._14, m[i]._21, m[i]._22, m[i]._23, m[i]._24, m[i]._31, m[i]._32,
                m[i]._33, m[i]._34, m[i]._41, m[i]._42, m[i]._43, m[i]._44);
    fclose(f);
    log("Character snapshot written to " + folder + "\\snapshot.txt");
}

// Binary frame: "CWA2" header once; then per tick: uint64 ms, int64[3] position (dots),
// int32 state, int32 timer, uint32 flags, int32 ability, float time_since_ability,
// float yaw, float[3] velocity, int32[14] sprite slots, float[33*16] matrices.
static std::unordered_map<void *, int> slotOf;
static ULONGLONG nextMap = 0;
static void refreshSlots(cube::World *w) {
    if (GetTickCount64() >= nextMap) {
        slotOf = slotMap(w);
        nextMap = GetTickCount64() + 5000;
    }
}
static void writeFrame(FILE *file, cube::Creature *c) {
    auto &e = c->entity_data;
    auto &s = c->animation_state;
    uint64_t ms = GetTickCount64();
    int32_t head[4] = {s.current_animation_state_id, s.current_animation_state_timer, (int32_t)e.flags,
                       (int32_t)e.current_ability};
    float values[5] = {e.time_since_ability, e.yaw, e.velocity.x, e.velocity.y, e.velocity.z};
    int32_t slots[SPRITES];
    int64_t *sprites = spritePointers(c);
    for (int i = 0; i < SPRITES; i++) {
        auto it = slotOf.find((void *)sprites[i]);
        slots[i] = it == slotOf.end() ? -1 : it->second;
    }
    fwrite(&ms, 8, 1, file);
    fwrite(&e.position, 8, 3, file);
    fwrite(head, 4, 4, file);
    fwrite(values, 4, 5, file);
    fwrite(slots, 4, SPRITES, file);
    fwrite(matrices(c), sizeof(Matrix4), MATRICES, file);
}
static void record(cube::World *w, cube::Creature *c) {
    if (!file)
        return;
    refreshSlots(w);
    writeFrame(file, c);
    recorded++;
}

static void toggle(cube::Creature *c) {
    recording = !recording;
    if (recording) {
        std::string path = characterFolder(c) + "\\anim-" + std::to_string(GetTickCount64()) + ".cwa";
        if (fopen_s(&file, path.c_str(), "wb") != 0)
            file = nullptr;
        if (file)
            fwrite("CWA2", 1, 4, file);
        recorded = 0;
        log("Animation recording started: " + path);
    } else {
        if (file)
            fclose(file);
        file = nullptr;
        log("Animation recording stopped: " + std::to_string(recorded) + " frames");
    }
}
// --- Creatures and NPCs ---
// While active (F3), every creature the game has around the player is recorded:
// its appearance and sprites once (creatures.txt), then its position and pose
// every second tick, up to CREATURE_FRAMES frames (c_<id>.cwa, same frames as
// the player's). The tour visits stops around home so the game spawns the
// creatures and NPCs of each area.
constexpr int CREATURE_FRAMES = 300;   // 10 s at 30 frames per second
constexpr int CREATURE_TOUR_RADIUS = 12; // candidate stops in each direction
constexpr int CREATURE_TOUR_STEP = 3;    // zones between candidates (192 blocks)
constexpr size_t CREATURE_TOUR_STOPS = 40; // nearest land stops visited
constexpr ULONGLONG CREATURE_STOP_MS = 12000;
static bool creatureMode = false;
struct Tracked {
    FILE *file = nullptr;
    int frames = 0;
    bool seen = false;
};
static std::unordered_map<int64_t, Tracked> tracked;
static std::set<int64_t> finished;
static std::string creatureFolder;
static unsigned creatureTick = 0;

static std::string spriteFile(void *sprite) {
    std::string n = sprite ? nameOf(sprite) : "";
    if (n.rfind("sprites/", 0) == 0)
        n = n.substr(8);
    if (n.size() > 4 && n.compare(n.size() - 4, 4, ".cub") == 0)
        n.resize(n.size() - 4);
    return n.empty() ? "-" : n;
}

static bool usablePose(cube::Creature *c) {
    // Newly spawned/offscreen creatures can have uninitialized AnimationState
    // data. Do not turn those into empty ten-second recordings.
    float m[16];
    memcpy(m, matrices(c), sizeof m);
    for (float v : m)
        if (!std::isfinite(v))
            return false;
    if (std::abs(m[12]) >= 1e5f || std::abs(m[13]) >= 1e5f || std::abs(m[14]) >= 1e5f)
        return false;
    if (std::abs(m[3]) > .001f || std::abs(m[7]) > .001f || std::abs(m[11]) > .001f ||
        std::abs(m[15] - 1) > .001f)
        return false;
    for (int axis = 0; axis < 3; axis++) {
        const float *v = m + axis * 4;
        float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (!(length > .005f && length < 1.f))
            return false;
    }
    return true;
}

static bool namedAppearance(cube::Creature *c) {
    auto sprites = spritePointers(c);
    // A named head or body is enough; several animals have just one of them.
    for (int slot : {0, 2})
        if (sprites[slot] && !nameOf((void *)sprites[slot]).empty())
            return true;
    return false;
}

static void describe(cube::Creature *c) {
    FILE *f = nullptr;
    if (fopen_s(&f, (creatureFolder + "\\creatures.txt").c_str(), "a") != 0 || !f)
        return;
    auto &e = c->entity_data;
    auto &a = e.appearance;
    std::string name(e.name, strnlen(e.name, sizeof e.name));
    for (auto &ch : name)
        if (ch == ' ' || ch == '\t')
            ch = '_';
    fprintf(f, "id=%lld race=%u hostility=%d level=%u class=%u name=%s hair=%d,%d,%d scale=%.4f flags2=%08x ",
            (long long)c->id, e.race, e.hostility_type, e.level, e.classType, name.empty() ? "-" : name.c_str(),
            a.hair_color.red, a.hair_color.green, a.hair_color.blue, a.graphics_scale, a.flags2);
    // The creature's own gear (11 slots: category:id:material:rarity:modifier),
    // which pairs the game's item ids with the sprites it draws for them.
    fprintf(f, "%s equip=", statsText(c).c_str());
    const cube::Item *gear = &c->entity_data.equipment.unk_item;
    for (int i = 0; i < 11; i++)
        fprintf(f, "%s%d:%d:%d:%d:%u", i ? "," : "", gear[i].category, gear[i].id, gear[i].material, gear[i].rarity, gear[i].modifier);
    fprintf(f, " sprites=");
    int64_t *sprites = spritePointers(c);
    for (int i = 0; i < SPRITES; i++)
        fprintf(f, "%s%s", i ? "," : "", spriteFile((void *)sprites[i]).c_str());
    fprintf(f, "\n");
    fclose(f);
}

static void stopTracking(int64_t id, Tracked &t) {
    if (t.file)
        fclose(t.file);
    t.file = nullptr;
    finished.insert(id);
}

// Diagnostics for the latest pass: creatures present, posed by the game, and named.
static size_t seenPresent = 0, seenPosed = 0, seenNamed = 0;
static void recordCreatures(cube::World *w) {
    if (!creatureMode || (++creatureTick & 1))
        return;
    seenPresent = seenPosed = seenNamed = 0;
    for (cube::Creature *c : w->creatures)
        if (c && c != w->local_creature) {
            seenPresent++;
            bool posed = usablePose(c);
            seenPosed += posed;
            seenNamed += posed && namedAppearance(c);
        }
    refreshSlots(w);
    for (auto &[id, t] : tracked)
        t.seen = false;
    for (cube::Creature *c : w->creatures) {
        if (!c || c == w->local_creature || finished.count(c->id))
            continue;
        auto it = tracked.find(c->id);
        if (it != tracked.end())
            it->second.seen = true;
        if (!usablePose(c))
            continue;
        if (it == tracked.end()) {
            if (!namedAppearance(c))
                continue;
            if (tracked.size() >= 96)
                continue; // Bounded number of open files.
            Tracked t;
            std::string path = creatureFolder + "\\c_" + std::to_string(c->id) + ".cwa";
            if (fopen_s(&t.file, path.c_str(), "wb") != 0 || !t.file)
                continue;
            fwrite("CWA2", 1, 4, t.file);
            describe(c);
            it = tracked.emplace(c->id, t).first;
        }
        it->second.seen = true;
        writeFrame(it->second.file, c);
        if (++it->second.frames >= CREATURE_FRAMES)
            stopTracking(it->first, it->second);
    }
    // Creatures that despawned keep the frames they have.
    for (auto it = tracked.begin(); it != tracked.end();) {
        if (!it->second.file || !it->second.seen) {
            if (it->second.file)
                stopTracking(it->first, it->second);
            it = tracked.erase(it);
        } else
            ++it;
    }
}

static void setCreatureMode(bool on, int seed) {
    creatureMode = on;
    if (on) {
        finished.clear();
        creatureFolder = "Extract\\seed_" + std::to_string(seed) + "\\creatures";
        // Creatures recorded before (their ids are the world's, kept across sessions)
        // are skipped, so a tour adds to the recordings instead of writing them twice.
        // [Tour] Rerecord=1 in Mods\ZoneExtractor.ini records them again.
        if (!GetPrivateProfileIntA("Tour", "Rerecord", 0, ".\\Mods\\ZoneExtractor.ini")) {
            FILE *known = nullptr;
            if (fopen_s(&known, (creatureFolder + "\\creatures.txt").c_str(), "r") == 0 && known) {
                char chunk[256];
                bool lineStart = true;
                while (fgets(chunk, sizeof chunk, known)) {
                    long long id;
                    if (lineStart && sscanf_s(chunk, "id=%lld", &id) == 1)
                        finished.insert(id);
                    lineStart = strchr(chunk, '\n') != nullptr;
                }
                fclose(known);
            }
        }
        CreateDirectoryA("Extract", nullptr);
        CreateDirectoryA(("Extract\\seed_" + std::to_string(seed)).c_str(), nullptr);
        CreateDirectoryA(creatureFolder.c_str(), nullptr);
        log("Creature recording started in " + creatureFolder + " (" + std::to_string(finished.size()) +
            " recorded before, skipped)");
    } else {
        for (auto &[id, t] : tracked)
            stopTracking(id, t);
        tracked.clear();
        log("Creature recording stopped: " + std::to_string(finished.size()) + " creatures recorded");
    }
}
} // namespace character

// --- Deaths and drops (fight mode) ---
// How the game shows a monster dying and what it drops, for CubeVulkan to match:
// from the moment a monster's HP reaches 0 until the game removes it, every tick
// of its pose (Extract\death\d_<race>_<id>.cwa, frames as the creature
// recordings) and its state (death-log.txt). Anything new the world holds after a
// death (pouches, coins) is found by watching the sizes of the world's containers
// and dumping new entries tick by tick (drops-log.txt), with screenshots
// (Extract\death\frame-*.bmp) over the first four seconds.
namespace deathprobe {
struct Watch {
    int64_t id = 0;
    FILE *frames = nullptr;
    ULONGLONG start = 0;
    LongVector3 at{};
};
static std::vector<Watch> watching;
static std::unordered_map<int64_t, bool> alive;
static FILE *text = nullptr, *drops = nullptr;
static ULONGLONG lastDeath = 0;
static LongVector3 lastAt{};
static std::set<uintptr_t> knownPointers, knownNodes;
static std::set<int64_t> knownCreatures;
static std::map<uintptr_t, ULONGLONG> dumping; // address of a new entry -> when it appeared
static std::string lastSizes;

static void open() {
    if (text)
        return;
    CreateDirectoryA("Extract", nullptr);
    CreateDirectoryA("Extract\\death", nullptr);
    text = _fsopen("Extract\\death\\death-log.txt", "a", _SH_DENYNO); // readable while the game runs
    drops = _fsopen("Extract\\death\\drops-log.txt", "a", _SH_DENYNO);
}
template <class T> static size_t bytes(const std::vector<T> &v) {
    auto raw = (char *const *)&v; // first, last, end
    return raw[1] - raw[0];
}
static bool readable(uintptr_t address, void *out, size_t n) {
    __try {
        memcpy(out, (const void *)address, n);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
static void dump(FILE *f, const char *what, uintptr_t address, ULONGLONG since) {
    unsigned char p[0x140];
    if (!readable(address, p, sizeof p))
        return;
    fprintf(f, "t=%llu %s %p +%llu:", (unsigned long long)GetTickCount64(), what, (void *)address,
            (unsigned long long)(GetTickCount64() - since));
    for (int i = 0; i < 0x140; i += 4) {
        float fv;
        int32_t iv;
        memcpy(&fv, p + i, 4);
        memcpy(&iv, p + i, 4);
        if (std::isfinite(fv) && std::abs(fv) > 1e-4f && std::abs(fv) < 1e7f)
            fprintf(f, " %x:%g", i, fv);
        else
            fprintf(f, " %x:#%d", i, iv);
    }
    fprintf(f, "\n");
}
static cube::Zone *zoneAt(cube::World *w, LongVector3 at) {
    IntVector2 key((int)(at.x >> 22), (int)(at.y >> 22)); // dots -> blocks (65536) -> zones (64)
    for (auto &[k, z] : w->zones)
        if (z && z->position.x == key.x && z->position.y == key.y)
            return z;
    return nullptr;
}

// The game's update lists (cube::ServerUpdates): what the local server tells the
// client to show (particles, pick-ups, sounds, hits). Another thread fills them, so
// each list is walked with guarded reads; new entries are logged raw and decoded.
static std::set<std::string> seenUpdates;
static void pollUpdates(FILE *f, const char *who, cube::ServerUpdates &u) {
    struct ListInfo {
        const char *name;
        void *list;
        size_t size;
    };
    ListInfo lists[] = {{"hit", &u.hit_packet_list, sizeof(cube::ServerUpdates::SU_HitPacket)},
                        {"sound", &u.sound_packet_list, sizeof(cube::ServerUpdates::SU_SoundPacket)},
                        {"particle", &u.particle_packet_list, sizeof(cube::ServerUpdates::SU_ParticlePacket)},
                        {"pickup", &u.pickup_item_packet_list, sizeof(cube::ServerUpdates::SU_PickupItemPacket)},
                        {"p9", &u.packet_list_9, sizeof(cube::ServerUpdates::SU_Packet9)},
                        {"p10", &u.packet_list_10, sizeof(cube::ServerUpdates::SU_Packet10)},
                        {"p11", &u.packet_list_11, sizeof(cube::ServerUpdates::SU_Packet11)},
                        {"p13", &u.packet_list_13, sizeof(cube::ServerUpdates::SU_Packet13)},
                        {"p14", &u.packet_list_14, sizeof(cube::ServerUpdates::SU_Packet14)}};
    for (auto &l : lists) {
        uintptr_t head = 0;
        if (!readable((uintptr_t)l.list, &head, 8) || !head)
            continue;
        uintptr_t node = 0;
        if (!readable(head, &node, 8))
            continue;
        for (int n = 0; n < 256 && node && node != head; n++) {
            unsigned char value[0x200];
            size_t bytes = std::min(l.size, sizeof value);
            if (!readable(node + 16, value, bytes))
                break;
            std::string key(l.name);
            key.append((const char *)value, bytes);
            if (seenUpdates.insert(key).second) {
                fprintf(f, "t=%llu %s %s:", (unsigned long long)GetTickCount64(), who, l.name);
                if (!strcmp(l.name, "particle")) {
                    auto *p = (cube::ServerUpdates::SU_ParticlePacket *)value;
                    fprintf(f, " pos=%.3f %.3f %.3f accel=%.3f %.3f %.3f color=%.3f %.3f %.3f %.3f f34=%d count=%d type=%d f40=%d",
                            p->pos.x / 65536.0, p->pos.y / 65536.0, p->pos.z / 65536.0, p->accel_maybe.x, p->accel_maybe.y,
                            p->accel_maybe.z, p->color.red, p->color.green, p->color.blue, p->color.alpha, p->field_34,
                            p->count, p->particle_type, p->field_40);
                } else if (!strcmp(l.name, "pickup")) {
                    auto *p = (cube::ServerUpdates::SU_PickupItemPacket *)value;
                    fprintf(f, " guid=%lld item=%d:%d:%d:%d quantity=%d pos=%.3f %.3f %.3f flags=%08x", (long long)p->guid,
                            (int)p->unk_item.category, p->unk_item.id, (int)p->unk_item.material, (int)p->unk_item.rarity,
                            p->quantity, p->pos.x / 65536.0, p->pos.y / 65536.0, p->pos.z / 65536.0, p->some_flag_things);
                } else if (!strcmp(l.name, "sound")) {
                    auto *p = (cube::ServerUpdates::SU_SoundPacket *)value;
                    fprintf(f, " id=%d pos=%.3f %.3f %.3f a=%.3f b=%.3f guid=%lld", p->sound_id, p->pos.x / 65536.0,
                            p->pos.y / 65536.0, p->pos.z / 65536.0, p->some_float_1, p->some_float_2, (long long)p->guid);
                }
                fprintf(f, " raw=");
                for (size_t i = 0; i < bytes; i++)
                    fprintf(f, "%02x", value[i]);
                fprintf(f, "\n");
            }
            if (!readable(node, &node, 8))
                break;
        }
    }
    if (seenUpdates.size() > 20000)
        seenUpdates.clear();
}

// Native body particles are nodes in Game+0x2090, not a contiguous array in
// writable memory. Reading this bounded list avoids the old multi-GB heap scan,
// and records actual age, velocity and spin rather than guessing from strides.
static void recordParticles(cube::Game *g) {
    ULONGLONG now = GetTickCount64();
    static ULONGLONG lastSample = 0;
    static FILE *file = nullptr;
    if (!lastDeath || now - lastDeath > 7000 || now - lastSample < 30)
        return;
    lastSample = now;
    if (!file)
        file = _fsopen("Extract\\death\\particles.txt", "a", _SH_DENYNO);
    if (!file)
        return;
    uintptr_t head = 0, node = 0;
    size_t count = 0;
    if (!readable((uintptr_t)g + 0x2090, &head, 8) ||
        !readable((uintptr_t)g + 0x2098, &count, 8) || count > 20000 ||
        !head || !readable(head, &node, 8))
        return;
    size_t logged = 0;
    for (size_t n = 0; n < 20000 && node && node != head; n++) {
        unsigned char data[0xc8];
        if (!readable(node, data, sizeof data))
            break;
        uintptr_t next;
        int kind, age, life, checkAge;
        memcpy(&next, data, 8);
        memcpy(&kind, data + 0x10, 4);
        memcpy(&age, data + 0x14, 4);
        memcpy(&life, data + 0xc4, 4);
        if (kind == 0 && life == 5000 && data[0xc0] && logged < 64 &&
            readable(node + 0x14, &checkAge, 4) && checkAge == age) {
            int64_t pos[3];
            float velocity[3], angles[3], spin[3];
            memcpy(pos, data + 0x18, sizeof pos);
            memcpy(velocity, data + 0x64, sizeof velocity);
            memcpy(angles, data + 0x48, sizeof angles);
            memcpy(spin, data + 0x70, sizeof spin);
            fprintf(file, "t=%llu node=%p age=%d life=%d pos=%lld,%lld,%lld velocity=%.9g,%.9g,%.9g "
                          "angles=%.9g,%.9g,%.9g spin=%.9g,%.9g,%.9g\n",
                    (unsigned long long)now, (void *)node, age, life,
                    (long long)pos[0], (long long)pos[1], (long long)pos[2],
                    velocity[0], velocity[1], velocity[2], angles[0], angles[1], angles[2],
                    spin[0], spin[1], spin[2]);
            logged++;
        }
        if (next == node)
            break;
        node = next;
    }
    fprintf(file, "t=%llu sample count=%zu body-particles=%zu\n", (unsigned long long)now, count, logged);
    fflush(file);
}

// Unnamed objects (the renderer, the game): every field shaped like a std::vector
// (begin <= end <= capacity, within 64 MB), its byte size logged when it changes in
// the seconds after a death, to find where the client keeps death effects.
static std::map<std::string, uint64_t> vectorSizes;
static void scanVectors(FILE *f, const char *name, uintptr_t object, size_t bytes, long long sinceDeath) {
    std::vector<uint64_t> v(bytes / 8);
    if (!object || !readable(object, v.data(), v.size() * 8))
        return;
    for (size_t i = 0; i + 2 < v.size(); i++) {
        uint64_t b = v[i], e = v[i + 1], c = v[i + 2];
        if (!b || b > e || e > c || c - b > (64ull << 20) || b < 0x10000 || (b & 7))
            continue;
        std::string key = std::string(name) + "+" + std::to_string(i * 8);
        auto it = vectorSizes.find(key);
        if (it != vectorSizes.end() && it->second != e - b && sinceDeath >= 0 && sinceDeath < 4000) {
            fprintf(f, "t=%llu since-death=%lld vector %s+0x%zx %llu -> %llu bytes\n", (unsigned long long)GetTickCount64(),
                    sinceDeath, name, i * 8, (unsigned long long)it->second, (unsigned long long)(e - b));
            // A vector of pointers that grew: what its new entries point to.
            if (e - b > it->second && e - b <= 4096 * 8) {
                std::vector<uint64_t> entries((e - b) / 8);
                if (readable(b, entries.data(), entries.size() * 8))
                    for (size_t k = it->second / 8; k < entries.size(); k++)
                        if (entries[k] > 0x10000 && !(entries[k] & 7) && !dumping.count(entries[k]))
                            dumping[entries[k]] = GetTickCount64(),
                            fprintf(f, "t=%llu new %s+0x%zx[%zu] -> %p\n", (unsigned long long)GetTickCount64(), name,
                                    i * 8, k, (void *)entries[k]);
            }
        }
        vectorSizes[key] = e - b;
    }
}

static void tick(cube::Game *g) {
    if (!combat::active)
        return;
    cube::World *w = g->world;
    open();
    if (drops) {
        long long since = lastDeath ? (long long)(GetTickCount64() - lastDeath) : -1;
        scanVectors(drops, "renderer", (uintptr_t)g->renderer, 0xA48, since);
        scanVectors(drops, "game", (uintptr_t)g, 0x2288, since);
        scanVectors(drops, "world", (uintptr_t)w, 0x548, since);
    }
    if (drops) {
        recordParticles(g);
        pollUpdates(drops, "client", w->server_updates);
        pollUpdates(drops, "server", g->host.world.server_updates);
    }
    cube::Creature *player = w->local_creature;
    open();
    if (!text || !drops)
        return;
    ULONGLONG now = GetTickCount64();
    character::refreshSlots(w);
    // Newly dead monsters.
    for (cube::Creature *c : w->creatures) {
        if (!c || c == player || c->entity_data.hostility_type != 1)
            continue;
        bool living = c->entity_data.HP > 0;
        bool was = alive.count(c->id) ? alive[c->id] : living;
        alive[c->id] = living;
        if (was && !living) {
            Watch watch;
            watch.id = c->id;
            watch.start = now;
            watch.at = c->entity_data.position;
            std::string path = "Extract\\death\\d_" + std::to_string(c->entity_data.race) + "_" + std::to_string(c->id) + ".cwa";
            if (fopen_s(&watch.frames, path.c_str(), "wb") == 0 && watch.frames)
                fwrite("CWA2", 1, 4, watch.frames);
            watching.push_back(watch);
            lastDeath = now;
            lastAt = watch.at;
            fprintf(text, "t=%llu death id=%lld race=%u stars=%u name=%.16s scale=%.4f file=%s\n", (unsigned long long)now,
                    (long long)c->id, c->entity_data.race, c->entity_data.level, c->entity_data.name,
                    c->entity_data.appearance.graphics_scale, path.c_str());
            uicapture::folder = "Extract\\death", uicapture::spacing = 100, uicapture::pending = 40; // one every 0.1 s for 4 s
            calibration::recordUntil = now + 3000;

        }
    }
    // The dying, tick by tick, until the game removes them.
    for (auto it = watching.begin(); it != watching.end();) {
        cube::Creature *c = nullptr;
        for (cube::Creature *o : w->creatures)
            if (o && o->id == it->id)
                c = o;
        if (!c || now - it->start > 30000) {
            fprintf(text, "t=%llu %s id=%lld after %llu ms\n", (unsigned long long)now, c ? "still-there" : "removed",
                    (long long)it->id, (unsigned long long)(now - it->start));
            if (it->frames)
                fclose(it->frames);
            it = watching.erase(it);
            continue;
        }
        auto &e = c->entity_data;
        auto &s = c->animation_state;
        if (it->frames)
            character::writeFrame(it->frames, c);
        fprintf(text,
                "t=%llu id=%lld +%llu hp=%.2f flags=%08x toggles=%04x pos=%.3f %.3f %.3f pitch=%.3f roll=%.3f yaw=%.3f "
                "vel=%.3f %.3f %.3f anim=%d/%d ability=%d since=%.0f hit=%.0f stun=%.0f f61=%d f62=%d f63=%d flags2=%08x "
                "gscale=%.4f\n",
                (unsigned long long)now, (long long)it->id, (unsigned long long)(now - it->start), e.HP, e.flags,
                (unsigned)(uint16_t)e.binary_toggles, e.position.x / 65536.0, e.position.y / 65536.0,
                e.position.z / 65536.0, e.pitch, e.roll, e.yaw, e.velocity.x, e.velocity.y, e.velocity.z,
                s.current_animation_state_id, s.current_animation_state_timer, e.current_ability, e.time_since_ability,
                e.time_since_hit, e.stun_time, e.field_61, e.field_62, e.field_63, e.appearance.flags2,
                e.appearance.graphics_scale);
        ++it;
    }
    // The world's containers: what grows when something is dropped.
    char sizes[512];
    bool locked = TryEnterCriticalSection(&w->zones_critical_section);
    cube::Zone *zone = locked && lastDeath ? zoneAt(w, lastAt) : nullptr;
    snprintf(sizes, sizeof sizes,
             "creatures=%zu list18=%zu projectiles=%zu dead=%zu rewards=%zu zones=%zu m190=%zu m1a0=%zu uv=%zu,%zu,%zu "
             "v360=%zu m378=%zu v388=%zu v3a0=%zu v4a0=%zu v4b8=%zu v4d0=%zu v4e8=%zu v500=%zu v518=%zu v530=%zu "
             "zone_list=%zu zone_list2=%zu",
             w->creatures.size(), w->unk_list_size_0x18.size(), w->projectiles.size(), w->state.some_map_size_0x28.size(),
             w->state.some_map_size_0x28_1.size(), w->state.some_map_size_0x88.size(), w->unk_map_0x190.size(),
             w->unk_map_0x1A0.size(), bytes(w->unk_vectors.unk_vec_0), bytes(w->unk_vectors.unk_vec_1),
             bytes(w->unk_vectors.unk_vec_2), bytes(w->unk_vec_0x360), w->unk_map_0x378.size(), bytes(w->field_388),
             bytes(w->field_3A0), bytes(w->field_4A0), bytes(w->field_4B8), bytes(w->field_4D0), bytes(w->field_4E8),
             bytes(w->field_500), bytes(w->field_518), bytes(w->field_530), zone ? zone->some_list.size() : 0,
             zone ? zone->some_list_2.size() : 0);
    if (sizes != lastSizes) {
        fprintf(drops, "t=%llu since-death=%lld %s\n", (unsigned long long)now,
                lastDeath ? (long long)(now - lastDeath) : -1LL, sizes);
        lastSizes = sizes;
    }
    // New entries: in the world's pointer list, the zone's lists, and new creatures near the corpse.
    for (void *p : w->unk_list_size_0x18)
        if (knownPointers.insert((uintptr_t)p).second && lastDeath)
            dumping[(uintptr_t)p] = now, fprintf(drops, "t=%llu new list18 entry %p\n", (unsigned long long)now, p);
    if (zone)
        for (auto *l : {&zone->some_list, &zone->some_list_2})
            for (auto &value : *l)
                if (knownNodes.insert((uintptr_t)&value).second && lastDeath)
                    dumping[(uintptr_t)&value] = now,
                    fprintf(drops, "t=%llu new zone %s entry %p\n", (unsigned long long)now,
                            l == &zone->some_list ? "list" : "list2", (void *)&value);
    if (locked)
        LeaveCriticalSection(&w->zones_critical_section);
    for (cube::Creature *c : w->creatures)
        if (c && knownCreatures.insert(c->id).second && lastDeath && now - lastDeath < 5000) {
            fprintf(drops, "t=%llu new creature id=%lld race=%u hostility=%d pos=%.3f %.3f %.3f\n", (unsigned long long)now,
                    (long long)c->id, c->entity_data.race, c->entity_data.hostility_type, c->entity_data.position.x / 65536.0,
                    c->entity_data.position.y / 65536.0, c->entity_data.position.z / 65536.0);
            Watch watch;
            watch.id = c->id, watch.start = now, watch.at = c->entity_data.position;
            std::string path = "Extract\\death\\spawn_" + std::to_string(c->entity_data.race) + "_" + std::to_string(c->id) + ".cwa";
            if (fopen_s(&watch.frames, path.c_str(), "wb") == 0 && watch.frames)
                fwrite("CWA2", 1, 4, watch.frames);
            watching.push_back(watch);
        }
    for (auto it = dumping.begin(); it != dumping.end();) {
        if (now - it->second > 10000) {
            it = dumping.erase(it);
            continue;
        }
        dump(drops, "entry", it->first, it->second);
        ++it;
    }
    fflush(text);
    fflush(drops);
}
} // namespace deathprobe

// --- Packet recorder (Extract\net.request) ---------------------------------------------
// Records the multiplayer packets this game handles, for CubeVulkan's game-compatible network
// layer (docs/specs/netcode-compatibility.md §5.1; checked with `EngineTests netdecode`).
//
// Hooks: the host's packet dispatcher 0xf4490 and the client's 0x3e450, both called as
// (host or client, sender SteamID, packet buffer), the buffer being a growable byte array (begin
// +0, end +8, read position i32 +0x18). Between them they see every packet the machine
// receives, through Steam and the loopback alike. While recording, SendP2PPacket (slot 0 of
// SteamNetworking005) is also hooked, for what the machine sends to others.
//
// Extract\net.request starts a recording into Extract\net\net-<date>-<time>.bin (it holds an
// optional duration in seconds, default 600); a request holding "stop" ends it. File layout
// (src/net/Recording.h): "CNR1", u32 1; then records: u32 size of the rest, u32 timeGetTime,
// u8 side (0 host received, 1 client received, 2 sent), u8 channel (0xff unknown), u16 peer
// (0 this machine, then 1, 2, ... in order of appearance: no SteamIDs are written), u32 the
// buffer's read position on entry, the packet's bytes. The game is only read, never changed.
namespace netrecorder {
constexpr uintptr_t HOST_DISPATCH_RVA = 0xf4490;
constexpr uintptr_t CLIENT_DISPATCH_RVA = 0x3e450;
constexpr size_t BUFFER_BEGIN = 0x00, BUFFER_END = 0x08, BUFFER_READ = 0x18;
constexpr uint8_t SIDE_HOST = 0, SIDE_CLIENT = 1, SIDE_SENT = 2;
constexpr uint8_t CHANNEL_UNKNOWN = 0xff;
constexpr uint32_t MAX_PACKET = 16u << 20;

using Dispatch = uint64_t(__fastcall *)(void *self, uint64_t sender, void *buffer, void *extra);
using SendP2P = bool(__fastcall *)(void *self, uint64_t to, const void *data, uint32_t size, int type, int channel);
static Dispatch originalHost = nullptr, originalClient = nullptr;
static SendP2P originalSend = nullptr;
static void **sendSlot = nullptr; // SteamNetworking005's method table entry we replaced

static std::atomic<bool> recording{false};
static std::mutex lock;
static FILE *out = nullptr;
static ULONGLONG stopAt = 0;
static uint32_t written = 0;
static uint64_t me = 0;
static std::map<uint64_t, uint16_t> peers;
static bool senderLogged = false;

// --- The game's SteamID test (netcode-transport.md, "valid SteamID") --------------------
static bool validSteamId(uint64_t id) {
    const uint32_t high = (uint32_t)(id >> 32), account = (uint32_t)id;
    const uint32_t universe = high >> 24, type = (high >> 20) & 0xf, instance = high & 0xfffff;
    if (universe < 1 || universe > 4 || type < 1 || type > 10)
        return false;
    if (type == 1)
        return account != 0 && instance < 5;
    if (type == 7)
        return account != 0 && instance == 0;
    if (type == 3)
        return account != 0;
    return true;
}

static bool readable(const void *p, size_t n) {
    MEMORY_BASIC_INFORMATION m{};
    if (!p || !VirtualQuery(p, &m, sizeof m) || m.State != MEM_COMMIT || (m.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
        return false;
    return (const uint8_t *)p + n <= (const uint8_t *)m.BaseAddress + m.RegionSize;
}

// The sender argument: a SteamID passed by value, or a pointer to one. Which it is was not
// traced in the spec, so both are accepted (the log says which was seen).
static uint64_t senderOf(uint64_t argument) {
    if (validSteamId(argument)) {
        if (!senderLogged)
            extract::log("Packet recorder: the sender comes as a SteamID value");
        senderLogged = true;
        return argument;
    }
    if (readable((const void *)argument, 8)) {
        const uint64_t id = *(const uint64_t *)argument;
        if (!senderLogged)
            extract::log(std::string("Packet recorder: the sender comes by pointer") + (validSteamId(id) ? "" : " (not a valid SteamID)"));
        senderLogged = true;
        return id;
    }
    return argument;
}

static uint16_t peerNumber(uint64_t id) {
    if (id == me)
        return 0;
    auto it = peers.find(id);
    if (it != peers.end())
        return it->second;
    const uint16_t n = (uint16_t)(peers.size() + 1);
    peers[id] = n;
    return n;
}

// Called with `lock` held.
static void write(uint8_t side, uint8_t channel, uint64_t peer, uint32_t readPosition, const uint8_t *data, uint32_t size) {
    if (!out)
        return;
    const uint32_t recordSize = 12 + size, time = timeGetTime();
    const uint16_t number = peerNumber(peer);
    fwrite(&recordSize, 4, 1, out);
    fwrite(&time, 4, 1, out);
    fwrite(&side, 1, 1, out);
    fwrite(&channel, 1, 1, out);
    fwrite(&number, 2, 1, out);
    fwrite(&readPosition, 4, 1, out);
    fwrite(data, 1, size, out);
    if (++written % 256 == 0)
        fflush(out);
}

static void recordReceived(uint8_t side, uint64_t senderArgument, void *buffer) {
    if (!recording.load() || !readable(buffer, BUFFER_READ + 4))
        return;
    auto *b = (uint8_t *)buffer;
    const uint8_t *begin = *(uint8_t **)(b + BUFFER_BEGIN), *end = *(uint8_t **)(b + BUFFER_END);
    const uint32_t readPosition = *(uint32_t *)(b + BUFFER_READ);
    if (!begin || end < begin || (size_t)(end - begin) > MAX_PACKET || !readable(begin, (size_t)(end - begin)))
        return;
    std::lock_guard<std::mutex> hold(lock);
    write(side, CHANNEL_UNKNOWN, senderOf(senderArgument), readPosition, begin, (uint32_t)(end - begin));
}

static uint64_t __fastcall hostHook(void *self, uint64_t sender, void *buffer, void *extra) {
    recordReceived(SIDE_HOST, sender, buffer);
    return originalHost(self, sender, buffer, extra);
}

static uint64_t __fastcall clientHook(void *self, uint64_t sender, void *buffer, void *extra) {
    recordReceived(SIDE_CLIENT, sender, buffer);
    return originalClient(self, sender, buffer, extra);
}

static bool __fastcall sendHook(void *self, uint64_t to, const void *data, uint32_t size, int type, int channel) {
    if (recording.load() && data && size <= MAX_PACKET) {
        std::lock_guard<std::mutex> hold(lock);
        write(SIDE_SENT, (uint8_t)channel, to, 0, (const uint8_t *)data, size);
    }
    return originalSend(self, to, data, size, type, channel);
}

// --- Detours on entry points whose first instructions are only known at run time ----------
// The length of one instruction of a function's prologue, for the kinds prologues use (push,
// mov/lea/sub/add/xor with a ModRM operand, movaps to the stack). 0 for anything else, and for
// instructions addressed relative to the instruction pointer, which could not be moved.
static size_t prologueInstructionLength(const uint8_t *p) {
    const size_t n = (p[0] & 0xf0) == 0x40 ? 1 : 0; // a REX prefix
    const uint8_t op = p[n];
    if (op >= 0x50 && op <= 0x57)
        return n + 1; // push
    auto modrm = [&](size_t at, size_t immediate) -> size_t {
        const uint8_t m = p[at], mod = m >> 6, rm = m & 7;
        size_t len = at + 1;
        if (mod != 3 && rm == 4)
            len++; // SIB
        if (mod == 0 && rm == 5)
            return 0; // relative to the instruction pointer
        if (mod == 0 && rm == 4 && (p[at + 1] & 7) == 5)
            len += 4; // SIB with a 32-bit displacement and no base
        if (mod == 1)
            len += 1;
        if (mod == 2)
            len += 4;
        return len + immediate;
    };
    switch (op) {
    case 0x89: case 0x8b: case 0x8d: case 0x03: case 0x2b: case 0x33: case 0x31: case 0x85:
        return modrm(n + 1, 0);
    case 0x83: return modrm(n + 1, 1);
    case 0x81: return modrm(n + 1, 4);
    case 0xc7: return modrm(n + 1, 4);
    case 0x0f:
        if (p[n + 1] == 0x29 || p[n + 1] == 0x11) // movaps / movups to memory
            return modrm(n + 2, 0);
        return 0;
    default: return 0;
    }
}

// Sends `rva` to `to`; returns a function that runs the original. Null (and a log line) when the
// prologue holds something that cannot be moved.
static void *detour(uintptr_t rva, void *to, const char *what) {
    auto *entry = (uint8_t *)(base() + rva);
    size_t length = 0;
    while (length < 5) {
        const size_t n = prologueInstructionLength(entry + length);
        if (n == 0 || length + n > 24) {
            char bytes[64];
            snprintf(bytes, sizeof bytes, "%02x %02x %02x %02x %02x %02x %02x %02x", entry[0], entry[1], entry[2],
                     entry[3], entry[4], entry[5], entry[6], entry[7]);
            extract::log(std::string("Packet recorder: cannot hook ") + what + " (starts " + bytes + ")");
            return nullptr;
        }
        length += n;
    }
    uint8_t *block = chatlog::nearBlock((uintptr_t)entry);
    if (!block)
        return nullptr;
    auto absoluteJump = [](uint8_t *at, uintptr_t dest) {
        at[0] = 0xff, at[1] = 0x25, *(uint32_t *)(at + 2) = 0;
        *(uintptr_t *)(at + 6) = dest;
    };
    memcpy(block, entry, length);
    absoluteJump(block + length, (uintptr_t)entry + length);
    absoluteJump(block + 48, (uintptr_t)to);
    DWORD was;
    if (!VirtualProtect(entry, length, PAGE_EXECUTE_READWRITE, &was))
        return nullptr;
    entry[0] = 0xe9;
    *(int32_t *)(entry + 1) = (int32_t)((intptr_t)(block + 48) - ((intptr_t)entry + 5));
    for (size_t i = 5; i < length; i++)
        entry[i] = 0x90;
    VirtualProtect(entry, length, was, &was);
    FlushInstructionCache(GetCurrentProcess(), entry, length);
    return block;
}

static void install() {
    if (!supported())
        return;
    originalHost = (Dispatch)detour(HOST_DISPATCH_RVA, (void *)&hostHook, "the host dispatcher 0xf4490");
    originalClient = (Dispatch)detour(CLIENT_DISPATCH_RVA, (void *)&clientHook, "the client dispatcher 0x3e450");
}

// The Steam objects this game uses, through its own steam_api64.dll.
static void *steamInterface(const char *version) {
    HMODULE steam = GetModuleHandleA("steam_api64.dll");
    if (!steam)
        return nullptr;
    auto user = (int(__cdecl *)())GetProcAddress(steam, "SteamAPI_GetHSteamUser");
    auto find = (void *(__cdecl *)(int, const char *))GetProcAddress(steam, "SteamInternal_FindOrCreateUserInterface");
    return user && find ? find(user(), version) : nullptr;
}

static void hookSend() {
    if (sendSlot)
        return;
    void *networking = steamInterface("SteamNetworking005");
    if (!networking)
        return;
    void **table = *(void ***)networking;
    DWORD was;
    if (!VirtualProtect(table, sizeof(void *), PAGE_READWRITE, &was))
        return;
    originalSend = (SendP2P)table[0];
    table[0] = (void *)&sendHook;
    VirtualProtect(table, sizeof(void *), was, &was);
    sendSlot = table;
}

static void findMe() {
    HMODULE steam = GetModuleHandleA("steam_api64.dll");
    void *user = steamInterface("SteamUser020");
    auto getId = steam ? (uint64_t(__cdecl *)(void *))GetProcAddress(steam, "SteamAPI_ISteamUser_GetSteamID") : nullptr;
    me = user && getId ? getId(user) : 0;
}

// Called with `lock` held.
static void stop() {
    recording = false;
    if (out) {
        fclose(out);
        out = nullptr;
        extract::log("Packet recorder: stopped after " + std::to_string(written) + " packets");
    }
}

static void poll() {
    if (!originalHost && !originalClient)
        return;
    if (recording.load() && GetTickCount64() >= stopAt) {
        std::lock_guard<std::mutex> hold(lock);
        stop();
    }
    if (GetFileAttributesA("Extract\\net.request") == INVALID_FILE_ATTRIBUTES)
        return;
    char text[32] = {};
    if (FILE *f = nullptr; fopen_s(&f, "Extract\\net.request", "r") == 0 && f) {
        fgets(text, sizeof text, f);
        fclose(f);
    }
    DeleteFileA("Extract\\net.request");
    std::lock_guard<std::mutex> hold(lock);
    stop();
    if (strncmp(text, "stop", 4) == 0)
        return;
    int seconds = atoi(text);
    if (seconds <= 0)
        seconds = 600;
    CreateDirectoryA("Extract\\net", nullptr);
    SYSTEMTIME t;
    GetLocalTime(&t);
    char name[96];
    snprintf(name, sizeof name, "Extract\\net\\net-%04d%02d%02d-%02d%02d%02d.bin", t.wYear, t.wMonth, t.wDay, t.wHour,
             t.wMinute, t.wSecond);
    if (fopen_s(&out, name, "wb") != 0 || !out) {
        out = nullptr;
        return;
    }
    const uint32_t version = 1;
    fwrite("CNR1", 1, 4, out);
    fwrite(&version, 4, 1, out);
    findMe();
    hookSend();
    peers.clear();
    written = 0;
    stopAt = GetTickCount64() + (ULONGLONG)seconds * 1000;
    recording = true;
    extract::log(std::string("Packet recorder: recording to ") + name + " for " + std::to_string(seconds) + " s" +
                 (sendSlot ? " (sends too)" : " (received packets only)"));
}
} // namespace netrecorder

// --- Native oracle ----------------------------------------------------------
// When <game>\Extract\oracle.request exists, the game's own creature functions
// are called with scripted rand() values and their results are written to
// Extract\oracle\, so CubeVulkan's reimplementation can be checked against them
// value by value (`EngineTests oracle`). Nothing is changed in the world.
namespace oracle {
constexpr uintptr_t RAND_IAT_RVA = 0x42cd28;   // the exe's import slot for rand()
constexpr uintptr_t SPAWN_CTOR_RVA = 0x118a60; // SpawnPoint *(SpawnPoint *)
constexpr uintptr_t LOOKS_RVA = 0x53e80;       // void (int *race, Appearance *, params *, char)
constexpr uintptr_t GEAR_RVA = 0x511e0;        // the spawn point's class, gear and abilities
constexpr uintptr_t RACE_LIST_RVA = 0x13f0d0;  // void (int, std::vector<int> *)
constexpr size_t SPAWN_SIZE = 0x950;

using Rand = int(__cdecl *)();
static Rand realRand = nullptr;
static DWORD scriptThread = 0;
static uint32_t scriptState = 0;
static uint32_t scriptCalls = 0;
// Scripted values: xorshift32 of a seed, 15 bits like the CRT's rand().
static uint32_t scripted() {
    scriptState ^= scriptState << 13;
    scriptState ^= scriptState >> 17;
    scriptState ^= scriptState << 5;
    return scriptState & 0x7fff;
}
// While the game generates a zone for the oracle, every rand() call on that thread is
// recorded: the caller's RVA and the value (generate() below, z_<x>_<y>.rtrace).
static DWORD traceThread = 0;
static std::vector<std::pair<uint32_t, uint16_t>> traced;
static int __cdecl hookedRand() {
    if (GetCurrentThreadId() == scriptThread) {
        scriptCalls++;
        return (int)scripted();
    }
    if (GetCurrentThreadId() == traceThread) {
        const int v = realRand();
        traced.push_back({(uint32_t)((uintptr_t)_ReturnAddress() - base()), (uint16_t)v});
        return v;
    }
    return realRand();
}
static bool hook(bool on) {
    void **slot = (void **)(base() + RAND_IAT_RVA);
    DWORD old;
    if (!VirtualProtect(slot, sizeof(void *), PAGE_READWRITE, &old))
        return false;
    if (on) {
        realRand = (Rand)*slot;
        *slot = (void *)&hookedRand;
    } else if (realRand)
        *slot = (void *)realRand;
    VirtualProtect(slot, sizeof(void *), old, &old);
    return true;
}
static void script(uint32_t seed) {
    scriptState = seed ? seed : 0x9e3779b9u;
    scriptCalls = 0;
}

template <class T> static void put(FILE *f, T v) { fwrite(&v, sizeof v, 1, f); }
struct IntVector {
    int *begin, *end, *cap;
};
static void putInts(FILE *f, const IntVector &v) {
    uint32_t n = v.begin && v.end > v.begin ? (uint32_t)(v.end - v.begin) : 0;
    put(f, n);
    if (n)
        fwrite(v.begin, 4, n, f);
}

static unsigned char *newSpawn() {
    auto *sp = (unsigned char *)calloc(1, SPAWN_SIZE); // leaked: the game's vectors inside it use the game's heap
    if (sp)
        ((void *(*)(void *))(base() + SPAWN_CTOR_RVA))(sp);
    return sp;
}

static bool looksOnce(unsigned char *sp, int race, uint32_t seed, uint32_t &calls, char wild = 0) {
    __try {
        *(int *)(sp + 0x20) = race;
        script(seed);
        ((void (*)(int *, void *, void *, char))(base() + LOOKS_RVA))((int *)(sp + 0x20), sp + 0x50, nullptr, wild);
        calls = scriptCalls;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// looks-wild.txt: "<race> <draws> <flags>" with 0x53e80's fourth argument set (wild groups).
static void looksWild(FILE *f) {
    for (int race = 0; race <= 320; race++) {
        unsigned char *sp = newSpawn();
        uint32_t calls = 0;
        if (sp && looksOnce(sp, race, (uint32_t)race * 7919u + 1, calls, 1))
            fprintf(f, "%d %u %u\n", race, calls, *(uint32_t *)(sp + 0x58));
    }
}

// looks.bin: "CWL1", then per record: int32 race, uint32 seed, uint32 rand calls,
// int32 race after, 0xb0 bytes of the spawn point from +0x50 (the appearance).
static void looks(FILE *f, std::map<int, uint32_t> &flags) {
    fwrite("CWL1", 1, 4, f);
    for (int race = 0; race <= 320; race++)
        for (uint32_t k = 0; k < 24; k++) {
            unsigned char *sp = newSpawn();
            if (!sp)
                return;
            uint32_t seed = (uint32_t)race * 7919u + k * 104729u + 1, calls = 0;
            if (!looksOnce(sp, race, seed, calls))
                continue;
            put<int32_t>(f, race);
            put<uint32_t>(f, seed);
            put<uint32_t>(f, calls);
            put<int32_t>(f, *(int *)(sp + 0x20));
            fwrite(sp + 0x50, 1, 0xb0, f);
            if (k == 0)
                flags[race] = *(uint32_t *)(sp + 0x58);
        }
}

static bool gearOnce(unsigned char *sp, int hostility, int race, int level, uint32_t flags, int cult, uint32_t seed,
                     uint32_t &calls) {
    __try {
        script(seed);
        ((void (*)(int, int, int, uint32_t, int, uint8_t *, uint8_t *, void *, void *, int *, void *, void *))(
            base() + GEAR_RVA))(hostility, race, level, flags, cult, sp + 0x34, sp + 0x35, sp + 0x110, sp + 0x8c0,
                                (int *)(sp + 0x8d8), sp + 0x8e0, sp + 0x7f0);
        calls = scriptCalls;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// gear.bin: "CWG2", then per record: inputs int32 hostility, race, level, cult, uint32 flags,
// uint8 class in, uint32 seed; outputs uint32 rand calls, uint8 class, uint8 specialization,
// 0x6e0 bytes of equipment, the two int vectors (count + values) and the int between them,
// then the inventory: uint32 tabs, per tab uint32 bytes and the raw ItemStacks.
static void gear(FILE *f, const std::map<int, uint32_t> &flags) {
    fwrite("CWG2", 1, 4, f);
    static const uint8_t classes[] = {0, 0, 0, 1, 2, 3, 4, 0x88, 0x8a, 0x8d, 0x8e, 0x8f, 0x90};
    uint32_t pick = 12345;
    auto next = [&](uint32_t n) {
        pick ^= pick << 13, pick ^= pick >> 17, pick ^= pick << 5;
        return pick % n;
    };
    for (int i = 0; i < 30000; i++) {
        unsigned char *sp = newSpawn();
        if (!sp)
            return;
        const int race = (int)next(321), hostility = (int)next(9), level = (int)next(4) ? 0 : (int)next(60);
        const int cult = (int)next(3) ? 0 : (int)next(23);
        auto known = flags.find(race);
        uint32_t fl = known == flags.end() ? 0 : known->second;
        if (next(3) == 0)
            fl |= 0x400000;
        if (next(8) == 0)
            fl |= 1u << next(32);
        const uint8_t cls = classes[next(sizeof classes)];
        sp[0x34] = cls;
        const uint32_t seed = 0x51ed2700u + (uint32_t)i * 2654435761u;
        uint32_t calls = 0;
        if (!gearOnce(sp, hostility, race, level, fl, cult, seed, calls))
            continue;
        put<int32_t>(f, hostility);
        put<int32_t>(f, race);
        put<int32_t>(f, level);
        put<int32_t>(f, cult);
        put<uint32_t>(f, fl);
        put<uint8_t>(f, cls);
        put<uint32_t>(f, seed);
        put<uint32_t>(f, calls);
        put<uint8_t>(f, sp[0x34]);
        put<uint8_t>(f, sp[0x35]);
        fwrite(sp + 0x110, 1, 0x6e0, f);
        putInts(f, *(IntVector *)(sp + 0x8c0));
        put<int32_t>(f, *(int *)(sp + 0x8d8));
        putInts(f, *(IntVector *)(sp + 0x8e0));
        struct Tab {
            unsigned char *begin, *end, *cap;
        };
        auto *tabs = (Tab *)(sp + 0x7f0);
        auto *t0 = *(Tab **)tabs, *t1 = *((Tab **)tabs + 1);
        uint32_t n = t0 && t1 > t0 ? (uint32_t)(t1 - t0) : 0;
        put(f, n);
        for (uint32_t t = 0; t < n; t++) {
            uint32_t bytes = t0[t].begin && t0[t].end > t0[t].begin ? (uint32_t)(t0[t].end - t0[t].begin) : 0;
            put(f, bytes);
            if (bytes)
                fwrite(t0[t].begin, 1, bytes, f);
        }
    }
}

static bool raceList(int s, IntVector &v) {
    __try {
        ((void (*)(int, IntVector *))(base() + RACE_LIST_RVA))(s, &v);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// regions.txt: 0x115df0's region roles, as the zone generator builds them, for every region
// that an extracted zone (Extract\seed_<n>\z_*.rec) lies in:
//   region <rx> <ry>
//   map <offset> <x> <y> <int at +0x24> <8 bytes at +0x28 as two ints>   (maps at +0x10/20/60/70)
//   special <x> <y> / marked <byte> / items <ints...>
struct MapNode {
    MapNode *left, *parent, *right;
    char colour, nil;
};
static bool rolesOnce(unsigned char *roles, cube::World *w, IntVector2 *region) {
    __try {
        ((void (*)(void *, void *, IntVector2 *))(base() + 0x115df0))(roles, w, region);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
static void regionRoles(FILE *f, cube::World *w, const std::string &zones) {
    std::set<std::pair<int, int>> regions;
    WIN32_FIND_DATAA data;
    HANDLE h = FindFirstFileA((zones + "\\z_*.rec").c_str(), &data);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            int x, y;
            if (sscanf_s(data.cFileName, "z_%d_%d.rec", &x, &y) == 2) {
                IntVector2 r{};
                ((void (*)(cube::World *, IntVector2 *, int, int))(base() + REGION_AT_RVA))(w, &r, x * 64 + 32, y * 64 + 32);
                regions.insert({r.x, r.y});
            }
        } while (FindNextFileA(h, &data));
        FindClose(h);
    }
    for (auto [rx, ry] : regions) {
        auto *roles = (unsigned char *)calloc(1, 0x400); // leaked: its maps use the game's heap
        IntVector2 region{rx, ry};
        if (!roles || !rolesOnce(roles, w, &region))
            continue;
        fprintf(f, "region %d %d\n", rx, ry);
        for (int off : {0x10, 0x20, 0x60, 0x70}) {
            MapNode *head = *(MapNode **)(roles + off);
            if (!head)
                continue;
            std::vector<MapNode *> stack{head->parent};
            while (!stack.empty()) {
                MapNode *n = stack.back();
                stack.pop_back();
                if (!n || n->nil)
                    continue;
                const unsigned char *b = (const unsigned char *)n;
                fprintf(f, "map %x %d %d %d %d %d\n", off, *(int *)(b + 0x1c), *(int *)(b + 0x20), *(int *)(b + 0x24),
                        *(int *)(b + 0x28), *(int *)(b + 0x2c));
                stack.push_back(n->left);
                stack.push_back(n->right);
            }
        }
        fprintf(f, "special %d %d\nmarked %d\nitems", *(int *)(roles + 0x80), *(int *)(roles + 0x84), roles[0x88]);
        IntVector items = *(IntVector *)(roles + 0x90);
        for (int *p = items.begin; p && p < items.end; p++)
            fprintf(f, " %d", *p);
        fprintf(f, "\n");
    }
}

// pickers.txt: "<rva> <seed> <rand calls> <race>" for the race pickers the region roles use,
// called with -1; then people-or-beasts (0x1408e0) once more for each culture 0..15, with the
// culture as a fifth number ("<rva> <seed> <rand calls> <race> <culture>"), since its list
// depends on it.
static bool pickOnce(uintptr_t rva, int argument, uint32_t seed, int &race, uint32_t &calls) {
    __try {
        script(seed);
        race = ((int (*)(int))(base() + rva))(argument);
        calls = scriptCalls;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
static void pickers(FILE *f) {
    for (uintptr_t rva : {0x141480, 0x1408e0, 0x142190, 0x142760, 0x13ffc0, 0x13feb0, 0x141390, 0x142000, 0x13fa50, 0x13fda0, 0x140490})
        for (uint32_t s = 1; s <= 600; s++) {
            int race = 0;
            uint32_t calls = 0;
            if (pickOnce(rva, -1, s * 2654435761u, race, calls))
                fprintf(f, "%x %u %u %d\n", (unsigned)rva, s * 2654435761u, calls, race);
        }
    for (int culture = 0; culture < 16; culture++)
        for (uint32_t s = 1; s <= 600; s++) {
            int race = 0;
            uint32_t calls = 0;
            if (pickOnce(0x1408e0, culture, s * 2654435761u, race, calls))
                fprintf(f, "%x %u %u %d %d\n", 0x1408e0u, s * 2654435761u, calls, race, culture);
        }
}

// races.txt: "<selector> <entries...>" for the race lists 0x1420f0 picks from.
static void raceLists(FILE *f) {
    std::vector<int> selectors{-0x18697};
    for (int s = -200; s <= 400; s++)
        selectors.push_back(s);
    for (int s : selectors) {
        IntVector v{};
        if (!raceList(s, v) || v.begin == v.end)
            continue;
        fprintf(f, "%d", s);
        for (int *p = v.begin; p < v.end; p++)
            fprintf(f, " %d", *p);
        fprintf(f, "\n");
    }
}

// spawn-table.txt: the world's spawn groups (world+0x1b0, 0x28-byte records built by
// 0x44b40): "<index> <kind> <biome mask> <spot mask> <types...>".
static void spawnTable(FILE *f, cube::World *w) {
    struct Group {
        IntVector types;
        uint32_t kind, biomes, spots, pad;
    };
    static_assert(sizeof(Group) == 0x28, "spawn group");
    // The spawner lives in the world at +0x1b0: world pointer, a vector of per-race
    // values (+8), the groups (+0x20).
    unsigned char *spawner = (unsigned char *)w + 0x1b0;
    IntVector values = *(IntVector *)(spawner + 8);
    if (values.begin && values.end > values.begin && values.end - values.begin < 4096) {
        fprintf(f, "values");
        for (int *p = values.begin; p < values.end; p++)
            fprintf(f, " %d", *p);
        fprintf(f, "\n");
    }
    Group **range = (Group **)(spawner + 0x20);
    if (!range[0] || range[1] < range[0] || range[1] - range[0] > 4096)
        return;
    for (Group *g = range[0]; g < range[1]; g++) {
        if (g->types.end < g->types.begin || g->types.end - g->types.begin > 4096)
            break;
        fprintf(f, "%d %u %u %u", (int)(g - range[0]), g->kind, g->biomes, g->spots);
        for (int *p = g->types.begin; p && p < g->types.end; p++)
            fprintf(f, " %d", *p);
        fprintf(f, "\n");
    }
}

// z_<x>_<y>.spawns: "CWS1", uint32 count, then the raw spawn points (0x950 bytes each;
// the pointers inside are meaningless) of the zone's map at +0x460, plus each one's two
// int vectors (count + values).
static bool writeSpawns(unsigned char *zone, const std::string &name) {
    struct Node {
        Node *left, *parent, *right;
        char colour, nil;
        char pad[6];
        int64_t key;
        unsigned char *value;
    };
    Node *head = *(Node **)(zone + 0x460);
    if (!head)
        return false;
    std::vector<unsigned char *> list;
    std::vector<Node *> stack{head->parent};
    while (!stack.empty()) { // in key order is not needed: the key is in the record (+0x38)
        Node *n = stack.back();
        stack.pop_back();
        if (!n || n->nil)
            continue;
        if (n->value)
            list.push_back(n->value);
        stack.push_back(n->left);
        stack.push_back(n->right);
    }
    FILE *f = nullptr;
    if (fopen_s(&f, name.c_str(), "wb") != 0 || !f)
        return false;
    fwrite("CWS1", 1, 4, f);
    put<uint32_t>(f, (uint32_t)list.size());
    for (unsigned char *sp : list) {
        fwrite(sp, 1, SPAWN_SIZE, f);
        putInts(f, *(IntVector *)(sp + 0x8c0));
        putInts(f, *(IntVector *)(sp + 0x8e0));
    }
    fclose(f);
    return true;
}
static int spawns(cube::World *w, const std::string &dir) {
    int zones = 0;
    if (!TryEnterCriticalSection(&w->zones_critical_section))
        return -1;
    for (auto &[key, zone] : w->zones)
        if (zone && writeSpawns((unsigned char *)zone, dir + "\\z_" + std::to_string(zone->position.x) + "_" +
                                                           std::to_string(zone->position.y) + ".spawns"))
            zones++;
    LeaveCriticalSection(&w->zones_critical_section);
    return zones;
}

// Extract\oracle\gen: zones the game generates afresh for Extract\gen.request (lines "x y"),
// made the way 0x2ae0c0 makes a zone (rand() seeded with y * 1024 + seed + x, 0x2f9400, then
// the generator 0x2ff6f0) but kept out of the world, so without the world's saved changes.
// Each gets z_<x>_<y>.cwz, .rec and .spawns as above; day.txt holds the world's day
// (world+8), which some places' encounters depend on. The zones are leaked (their vectors
// live on the game's heap); a test run generates a few hundred at most.
constexpr uintptr_t SRAND_IAT_RVA = 0x42cd38; // the exe's import slot for srand()
constexpr uintptr_t NEW_RVA = 0x392bac;       // the game's operator new
constexpr uintptr_t ZONE_CTOR_RVA = 0x2f9400; // Zone *(Zone *, World *, IntVector2 *)
constexpr uintptr_t ZONE_GEN_RVA = 0x2ff6f0;  // void (Zone *)
constexpr size_t ZONE_SIZE = 0x58518;
static bool generateOnce(cube::World *w, int x, int y, unsigned char *&zone) {
    __try {
        auto srandFn = *(void(__cdecl **)(unsigned))(base() + SRAND_IAT_RVA);
        srandFn((unsigned)(y * 0x400 + *(int *)((unsigned char *)w + 0x244) + x));
        zone = (unsigned char *)((void *(*)(size_t))(base() + NEW_RVA))(ZONE_SIZE);
        if (!zone)
            return false;
        IntVector2 pos{x, y};
        ((void *(*)(void *, void *, IntVector2 *))(base() + ZONE_CTOR_RVA))(zone, w, &pos);
        ((void (*)(void *))(base() + ZONE_GEN_RVA))(zone);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
static void writeFile(const std::string &name, const std::vector<unsigned char> &data) {
    FILE *f = nullptr;
    if (fopen_s(&f, name.c_str(), "wb") == 0 && f) {
        fwrite(data.data(), 1, data.size(), f);
        fclose(f);
    }
}
// The daily refresh (0x31c540) the game gives a zone when it loads it (a new zone's day is -1).
// When it changes the zone's objects (a campfire) the game queues the zone for sending
// (world+0x1a0); the zone stays valid (it is leaked), so that is harmless.
constexpr uintptr_t ZONE_REFRESH_RVA = 0x31c540;
static bool refreshOnce(unsigned char *zone) {
    __try {
        ((void (*)(void *))(base() + ZONE_REFRESH_RVA))(zone);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
// A zone's blocks, records, spawn points and the rand() calls just traced, at stem.*.
static void writeZone(cube::World *w, unsigned char *zone, const std::string &stem) {
    writeFile(stem + ".cwz", extract::serialise(w, (cube::Zone *)zone));
    writeFile(stem + ".rec", extract::serialiseRecords((cube::Zone *)zone));
    writeSpawns(zone, stem + ".spawns");
    std::vector<unsigned char> trace(traced.size() * 6);
    for (size_t i = 0; i < traced.size(); i++) {
        memcpy(&trace[i * 6], &traced[i].first, 4);
        memcpy(&trace[i * 6 + 4], &traced[i].second, 2);
    }
    writeFile(stem + ".rtrace", trace); // (uint32 caller RVA, uint16 value) per call
}
static std::deque<std::pair<int, int>> genQueue; // a few a tick, so the game keeps running
static int genDone = 0;
static void queueGeneration(cube::World *w) {
    FILE *f = nullptr;
    if (fopen_s(&f, "Extract\\gen.request", "r") != 0 || !f)
        return;
    int x, y;
    while (fscanf_s(f, "%d %d", &x, &y) == 2)
        genQueue.push_back({x, y});
    fclose(f);
    DeleteFileA("Extract\\gen.request");
    CreateDirectoryA("Extract\\oracle", nullptr);
    CreateDirectoryA("Extract\\oracle\\gen", nullptr);
    CreateDirectoryA("Extract\\oracle\\gen\\refreshed", nullptr);
    if (fopen_s(&f, "Extract\\oracle\\gen\\day.txt", "w") == 0 && f) {
        fprintf(f, "%d\n", *(int *)((unsigned char *)w + 8));
        fclose(f);
    }
    genDone = 0;
    log("Oracle: generating " + std::to_string(genQueue.size()) + " zones into Extract\\oracle\\gen");
}
static void generate(cube::World *w) {
    for (int n = 0; n < 2 && !genQueue.empty(); n++) {
        auto [zx, zy] = genQueue.front();
        genQueue.pop_front();
        unsigned char *zone = nullptr;
        traced.clear();
        const bool tracing = hook(true);
        traceThread = GetCurrentThreadId();
        const bool made = generateOnce(w, zx, zy, zone);
        traceThread = 0;
        if (tracing)
            hook(false);
        if (!made) {
            log("Oracle: generating zone " + std::to_string(zx) + " " + std::to_string(zy) + " failed");
            continue;
        }
        const std::string name = "z_" + std::to_string(zx) + "_" + std::to_string(zy);
        writeZone(w, zone, "Extract\\oracle\\gen\\" + name);
        // The same zone after the refresh the game gives it on loading, in gen\refreshed.
        traced.clear();
        const bool tracing2 = hook(true);
        traceThread = GetCurrentThreadId();
        const bool refreshed = refreshOnce(zone);
        traceThread = 0;
        if (tracing2)
            hook(false);
        if (refreshed)
            writeZone(w, zone, "Extract\\oracle\\gen\\refreshed\\" + name);
        else
            log("Oracle: refreshing zone " + std::to_string(zx) + " " + std::to_string(zy) + " failed");
        if (++genDone % 25 == 0 || genQueue.empty())
            log("Oracle: generated " + std::to_string(genDone) + " zones" + (genQueue.empty() ? " (done)" : ""));
    }
}

// items.bin ("items.request"): the item generators a death's drop uses, on scripted rand():
// "CIG1", the world's special-item lists (+0x360: count, ids; +0x378: count, then key,
// count, ids), then per call: uint8 which (0: weapons 0x2d15d0, 1: armour 0x2d0680,
// 2: specials 0x2d0fc0), int32 class (the key for specials), uint8 rarity, uint8 plain,
// uint32 seed, uint32 rand calls, the item (0xa0 bytes).
static bool makeItem(int which, uint8_t *w, uint8_t *item, uint8_t rarity, int cls, char plain) {
    using Weapon = void *(*)(void *, uint8_t *, uint8_t, int);
    using Armour = void *(*)(void *, uint8_t *, uint8_t, int, char);
    using Special = void *(*)(void *, uint8_t *, int);
    __try {
        if (which == 0)
            ((Weapon)(base() + 0x2d15d0))(w, item, rarity, cls);
        else if (which == 1)
            ((Armour)(base() + 0x2d0680))(w, item, rarity, cls, plain);
        else
            ((Special)(base() + 0x2d0fc0))(w, item, cls);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
static void items(FILE *f, uint8_t *w) {
    fwrite("CIG1", 1, 4, f);
    {
        auto *b = *(int32_t **)(w + 0x360), *e = *(int32_t **)(w + 0x368);
        uint32_t n = b ? (uint32_t)(e - b) : 0;
        put(f, n);
        if (n)
            fwrite(b, 4, n, f);
        const auto nodes = updateprobe::treeNodes(*(uint8_t **)(w + 0x378));
        put(f, (uint32_t)nodes.size());
        for (uint8_t *node : nodes) {
            auto *vb = *(int32_t **)(node + 0x28), *ve = *(int32_t **)(node + 0x30);
            uint32_t m = vb ? (uint32_t)(ve - vb) : 0;
            put(f, *(int32_t *)(node + 0x20)), put(f, m);
            if (m)
                fwrite(vb, 4, m, f);
        }
    }
    const int classes[] = {-1, 0, 1, 2, 3, 4, 5};
    uint32_t seed = 0x1234567u;
    for (int which = 0; which < 3; which++)
        for (int i = 0; i < 2000; i++) {
            seed = seed * 1664525u + 1013904223u;
            const int cls = which == 2 ? (int)(seed >> 8) % 4 - 1 : classes[(seed >> 8) % 7];
            const uint8_t rarity = (uint8_t)((seed >> 16) % 6);
            const char plain = (char)((seed >> 20) & 1);
            uint8_t item[0xa0];
            memset(item, 0xcc, sizeof item);
            script(seed);
            if (!makeItem(which, w, item, rarity, cls, plain))
                continue;
            put(f, (uint8_t)which), put(f, (int32_t)cls), put(f, rarity), put(f, (uint8_t)plain);
            put(f, seed), put(f, scriptCalls);
            fwrite(item, 1, sizeof item, f);
        }
}

// stats.request: the local player's record and buffs with the game's own numbers for
// them, appended to Extract\oracle\stats.bin ("CST1" at the start; then per request:
// the record, uint32 buffs and 0x18 bytes each, then floats: max health 0x5fac0, attack
// 0x4fa70(1), spell 0x65c70(1), armor 0x4f080, resistance 0x64d90, critical
// 0x50d30(none, 1), attack 0x4fa70(0), spell 0x65c70(0), haste 0x66460, regen 0x647b0).
static void statsSample(cube::World *w) {
    if (GetFileAttributesA("Extract\\stats.request") == INVALID_FILE_ATTRIBUTES)
        return;
    DeleteFileA("Extract\\stats.request");
    cube::Creature *c = w->local_creature;
    if (!c)
        return;
    CreateDirectoryA("Extract\\oracle", nullptr);
    const bool fresh = GetFileAttributesA("Extract\\oracle\\stats.bin") == INVALID_FILE_ATTRIBUTES;
    FILE *f = nullptr;
    if (fopen_s(&f, "Extract\\oracle\\stats.bin", "ab") != 0 || !f)
        return;
    if (fresh)
        fwrite("CST1", 1, 4, f);
    std::vector<uint8_t> r;
    updateprobe::putBytes(r, c, updateprobe::CREATURE_SIZE);
    const auto buffs = updateprobe::listOf(*(void **)((uint8_t *)c + 0x980));
    updateprobe::put(r, (uint32_t)buffs.size());
    for (uint8_t *n : buffs)
        updateprobe::putBytes(r, n + 0x10, 0x18);
    auto call = [&](uintptr_t rva) { return ((float (*)(cube::Creature *))(base() + rva))(c); };
    auto callBool = [&](uintptr_t rva, bool b) { return ((float (*)(cube::Creature *, bool))(base() + rva))(c, b); };
    const float v[10] = {call(0x5fac0), callBool(0x4fa70, true), callBool(0x65c70, true), call(0x4f080), call(0x64d90),
                         ((float (*)(cube::Creature *, cube::Creature *, bool))(base() + 0x50d30))(c, nullptr, true),
                         callBool(0x4fa70, false), callBool(0x65c70, false), call(0x66460), call(0x647b0)};
    r.insert(r.end(), (const uint8_t *)v, (const uint8_t *)v + sizeof v);
    fwrite(r.data(), 1, r.size(), f);
    fclose(f);
    log("Stats oracle: sample written (max health " + std::to_string(v[0]) + ")");
}

static void run(cube::World *w) {
    animprobe::poll();
    statsSample(w);
    updateprobe::poll(w);
    hitoracle::poll();
    effectoracle::poll();
    droporacle::poll();
    controloracle::poll();
    controloracle::pollAttacks();
    aiprobe::poll(w);
    writewatch::poll(w);
    if (GetFileAttributesA("Extract\\gen.request") != INVALID_FILE_ATTRIBUTES)
        queueGeneration(w);
    generate(w);
    if (GetFileAttributesA("Extract\\items.request") != INVALID_FILE_ATTRIBUTES) {
        DeleteFileA("Extract\\items.request");
        CreateDirectoryA("Extract\\oracle", nullptr);
        FILE *f = nullptr;
        scriptThread = GetCurrentThreadId();
        if (fopen_s(&f, "Extract\\oracle\\items.bin", "wb") == 0 && f && hook(true)) {
            items(f, (uint8_t *)w);
            hook(false);
            fclose(f);
            log("Oracle: item generators written to Extract\\oracle\\items.bin");
        } else if (f)
            fclose(f);
        scriptThread = 0;
    }
    if (GetFileAttributesA("Extract\\oracle.request") == INVALID_FILE_ATTRIBUTES)
        return;
    DeleteFileA("Extract\\oracle.request");
    CreateDirectoryA("Extract\\oracle", nullptr);
    FILE *f = nullptr;
    if (fopen_s(&f, "Extract\\oracle\\spawn-table.txt", "w") == 0 && f) {
        spawnTable(f, w);
        fclose(f);
    }
    if (fopen_s(&f, "Extract\\oracle\\races.txt", "w") == 0 && f) {
        raceLists(f);
        fclose(f);
    }
    if (fopen_s(&f, "Extract\\oracle\\regions.txt", "w") == 0 && f) {
        regionRoles(f, w, "Extract\\seed_" + std::to_string(w->seed));
        fclose(f);
    }
    const int zones = spawns(w, "Extract\\oracle");
    scriptThread = GetCurrentThreadId();
    if (!hook(true)) {
        log("Oracle: could not hook rand()");
        return;
    }
    std::map<int, uint32_t> flags;
    if (fopen_s(&f, "Extract\\oracle\\looks.bin", "wb") == 0 && f) {
        looks(f, flags);
        fclose(f);
    }
    if (fopen_s(&f, "Extract\\oracle\\pickers.txt", "w") == 0 && f) {
        pickers(f);
        fclose(f);
    }
    if (fopen_s(&f, "Extract\\oracle\\looks-wild.txt", "w") == 0 && f) {
        looksWild(f);
        fclose(f);
    }
    if (fopen_s(&f, "Extract\\oracle\\gear.bin", "wb") == 0 && f) {
        gear(f, flags);
        fclose(f);
    }
    hook(false);
    scriptThread = 0;
    log("Oracle: written to Extract\\oracle (" + std::to_string(zones) + " zones' spawn points)");
}
} // namespace oracle

// --- The sound list (Extract\sounds.txt) -------------------------------------------------------
// The game numbers its 125 sounds (docs/specs/sound.md §1.2): 0xa3a40(game, n) names number n's
// file and asks the sound engine (game +0x28) to load it through its slot +0x40, which takes
// the file name. CubeVulkan plays the game's sound requests by number, so it needs the
// owner's list: for each n the engine's slot is briefly replaced by one that notes the name
// it is given (then loads as before; loading is cached by name). Written once, when the file
// is missing, or again when Extract\sounds.request appears. Lines: "<number> <file name>".
namespace soundlist {
constexpr uintptr_t SOUND_BY_NUMBER_RVA = 0xa3a40; // (game, number) -> handle
constexpr size_t ENGINE = 0x28;                     // game +0x28: the sound engine
constexpr size_t LOAD_SLOT = 0x40 / 8;              // its vtable slot +0x40: load by name
constexpr size_t SLOTS_COPIED = 16;
constexpr int SOUNDS = 125;

using Load = uint64_t (*)(void *, void *, void *, void *);
static Load original = nullptr;
static std::string noted;

// A name as the engine might be given it: an MSVC std::wstring or std::string (inline up to
// 7 or 15 characters), or a plain wide or narrow string. True when it ends in ".wav".
static bool endsWav(const std::string &s) {
    if (s.size() < 4)
        return false;
    std::string end = s.substr(s.size() - 4);
    for (char &ch : end)
        ch = (char)tolower((unsigned char)ch);
    return end == ".wav";
}
static std::string wideText(const void *p, size_t maxChars) {
    std::string out;
    for (size_t i = 0; i < maxChars; i++) {
        wchar_t ch = 0;
        animprobe::readSafe((const wchar_t *)p + i, &ch, sizeof ch);
        if (!ch || ch > 0x7e || ch < 0x20)
            break;
        out += (char)ch;
    }
    return out;
}
static std::string narrowText(const void *p, size_t maxChars) {
    std::string out;
    for (size_t i = 0; i < maxChars; i++) {
        char ch = 0;
        animprobe::readSafe((const char *)p + i, &ch, 1);
        if (!ch || ch > 0x7e || ch < 0x20)
            break;
        out += ch;
    }
    return out;
}
static std::string nameIn(void *arg) {
    if (!arg)
        return {};
    uint64_t words[4] = {};
    animprobe::readSafe(arg, words, sizeof words);
    const uint64_t size = words[2], capacity = words[3];
    if (size && size < 260 && capacity >= size) {
        // std::wstring: inline while the capacity is under 8, else a pointer.
        std::string w = wideText(capacity < 8 ? arg : (void *)words[0], (size_t)size);
        if (endsWav(w))
            return w;
        std::string n = narrowText(capacity < 16 ? arg : (void *)words[0], (size_t)size);
        if (endsWav(n))
            return n;
    }
    std::string w = wideText(arg, 260);
    if (endsWav(w))
        return w;
    std::string n = narrowText(arg, 260);
    return endsWav(n) ? n : std::string();
}
static uint64_t noting(void *engine, void *a, void *b, void *c) {
    for (void *arg : {a, b, c})
        if (noted.empty() && (uintptr_t)arg > 0x10000)
            noted = nameIn(arg);
    return original(engine, a, b, c);
}

static void write(cube::Game *g) {
    void *engine = *(void **)((uint8_t *)g + ENGINE);
    if (!engine)
        return;
    void **table = *(void ***)engine;
    void *copy[SLOTS_COPIED];
    memcpy(copy, table, sizeof copy);
    original = (Load)table[LOAD_SLOT];
    copy[LOAD_SLOT] = (void *)&noting;
    *(void ***)engine = copy;
    auto byNumber = (uint64_t(*)(cube::Game *, int))(base() + SOUND_BY_NUMBER_RVA);
    std::vector<std::string> names(SOUNDS);
    for (int n = 0; n < SOUNDS; n++) {
        noted.clear();
        byNumber(g, n);
        const size_t slash = noted.find_last_of("\\/");
        names[n] = slash == std::string::npos ? noted : noted.substr(slash + 1);
    }
    *(void ***)engine = table;
    CreateDirectoryA("Extract", nullptr);
    FILE *f = nullptr;
    int found = 0;
    if (fopen_s(&f, "Extract\\sounds.txt", "w") == 0 && f) {
        for (int n = 0; n < SOUNDS; n++)
            if (!names[n].empty())
                fprintf(f, "%d %s\n", n, names[n].c_str()), found++;
        fclose(f);
    }
    log("Sound list: " + std::to_string(found) + " of " + std::to_string(SOUNDS) + " numbers named in Extract\\sounds.txt");
}

static void run(cube::Game *g) {
    static bool checked = false;
    const bool asked = GetFileAttributesA("Extract\\sounds.request") != INVALID_FILE_ATTRIBUTES;
    const bool missing = !checked && GetFileAttributesA("Extract\\sounds.txt") == INVALID_FILE_ATTRIBUTES;
    checked = true;
    if (!asked && !missing)
        return;
    DeleteFileA("Extract\\sounds.request");
    write(g);
}
} // namespace soundlist

static void tick(cube::Game *g) {
    netrecorder::poll(); // also outside a world, so a recording can start before joining
    if (!g || !g->world || !g->world->local_creature || !supported())
        return;
    oracle::run(g->world);
    soundlist::run(g);
    {
        static bool f5 = false, f4 = false;
        bool down5 = GetAsyncKeyState(VK_F5) & 0x8000, down4 = GetAsyncKeyState(VK_F4) & 0x8000;
        if (down5 && !f5)
            character::snapshot(g->world, g->world->local_creature);
        if (down4 && !f4)
            character::toggle(g->world->local_creature);
        f5 = down5;
        f4 = down4;
        if (character::recording)
            character::record(g->world, g->world->local_creature);
        static bool f2 = false;
        bool down2 = GetAsyncKeyState(VK_F2) & 0x8000;
        if (down2 && !f2)
            combat::toggle(g->world);
        f2 = down2;
        combat::tick(g);
        static bool zero = false;
        bool downZero = GetAsyncKeyState('0') & 0x8000;
        if (downZero && !zero)
            aitrace::toggle();
        zero = downZero;
        static bool equals = false;
        bool downEquals = GetAsyncKeyState(VK_OEM_PLUS) & 0x8000;
        if (downEquals && !equals && aitrace::active)
            aitrace::startApproach(g->world);
        equals = downEquals;
        static bool minus = false;
        bool downMinus = GetAsyncKeyState(VK_OEM_MINUS) & 0x8000;
        if (downMinus && !minus)
            aitrace::visitTownsperson(g);
        minus = downMinus;
        static bool kKey = false;
        bool downK = GetAsyncKeyState('K') & 0x8000;
        if (downK && !kKey)
            talkscripts::exportAll(g, false);
        kKey = downK;
        static bool oKey = false;
        bool downO = GetAsyncKeyState('O') & 0x8000;
        if (downO && !oKey) {
            genoracle::dump(g);
            blockprobe::dump();
        }
        oKey = downO;
        aitrace::tick(g);
        static ULONGLONG nextScripts = 0;
        if (GetTickCount64() >= nextScripts) {
            nextScripts = GetTickCount64() + 2000;
            talkscripts::exportAll(g, true);
        }
        aitrace::approach(g->world);
        deathprobe::tick(g);
        gear::dumpFunctions();
        static bool f8 = false;
        bool down8 = GetAsyncKeyState(VK_F8) & 0x8000;
        if (down8 && !f8) {
            if (gear::active)
                gear::finish(g->world, "stopped");
            else {
                gear::classRules(g->world->local_creature);
                gear::begin(g->world);
            }
        }
        f8 = down8;
        gear::tick(g->world);
        static bool f7 = false;
        bool down7 = GetAsyncKeyState(VK_F7) & 0x8000;
        if (down7 && !f7)
            switchSpecialization(g->world->local_creature);
        f7 = down7;
        static bool ins = false;
        bool downIns = GetAsyncKeyState('X') & 0x8000;
        if (downIns && !ins)
            fillMana(g->world->local_creature);
        ins = downIns;
        static bool noon = false; // N: midday, for recordings that need daylight
        bool downN = GetAsyncKeyState('N') & 0x8000;
        if (downN && !noon) {
            g->world->state.time = g->host.world.state.time = 12 * 60 * 60 * 1000.f; // the local server keeps the clock
            log("Time set to 12:00");
        }
        noon = downN;
        static bool hunt = false; // H: 100 blocks on along the camera, onto the ground, to find monsters
        bool downH = GetAsyncKeyState('H') & 0x8000;
        if (downH && !hunt) {
            auto &p = g->world->local_creature->entity_data.position;
            double yaw = (g->camera_angle.z - 90) * 3.14159265358979 / 180;
            char gx[32] = {}, gy[32] = {}; // [Hunt] X, Y (blocks): go straight there instead
            GetPrivateProfileStringA("Hunt", "X", "", gx, sizeof gx, ".\\Mods\\ZoneExtractor.ini");
            GetPrivateProfileStringA("Hunt", "Y", "", gy, sizeof gy, ".\\Mods\\ZoneExtractor.ini");
            if (gx[0] && gy[0])
                hold(g->world, g->world->local_creature, _atoi64(gx), _atoi64(gy));
            else
                hold(g->world, g->world->local_creature, (p.x >> 16) + (int64_t)(100 * std::cos(yaw)),
                     (p.y >> 16) + (int64_t)(100 * std::sin(yaw)));
            int monsters = 0;
            for (cube::Creature *c : g->world->creatures)
                monsters += c && c->entity_data.hostility_type == 1 && c->entity_data.HP > 0;
            log("Hunt step: " + std::to_string(monsters) + " monsters simulated");
        }
        hunt = downH;
        // V: for 6 s, fight mode's target kept at 0.01 HP, so the player's next hit kills
        // it the game's own way (HP alone does not change how it dies or what it drops).
        static bool weaken = false;
        static ULONGLONG weakUntil = 0;
        bool downV = GetAsyncKeyState('V') & 0x8000;
        if (downV && !weaken)
            weakUntil = GetTickCount64() + 6000;
        if (GetTickCount64() < weakUntil && combat::targetId) {
            for (cube::World *world : {g->world, &g->host.world}) // the local server's copy decides
                for (cube::Creature *c : world->creatures)
                    if (c && c->id == combat::targetId && c->entity_data.HP > .01f)
                        c->entity_data.HP = .01f;
            if (!weaken)
                log("Holding the target at 0.01 HP");
        }
        weaken = downV;
        // 9: collision log (read-only). Every tick the player moves: position (blocks),
        // velocity and flags, to Extract\collision.txt, for measuring how close to walls,
        // ledges and ceilings the game lets the body go.
        // 8: guard (test harness, writes HP): the player stays at full health, so
        // measurements are not cut short by monsters. Restore the save afterwards.
        static bool guardKey = false, guarding = false;
        bool down8key = GetAsyncKeyState('8') & 0x8000;
        if (down8key && !guardKey) {
            guarding = !guarding;
            log(guarding ? "Guard on (full health)" : "Guard off");
        }
        guardKey = down8key;
        if (guarding) {
            cube::Creature *p = g->world->local_creature;
            float full = ((float (*)(cube::Creature *))(base() + MAX_HP_RVA))(p);
            if (p->entity_data.HP > 0 && p->entity_data.HP < full)
                p->entity_data.HP = full;
        }
        // 7: push (test harness, writes velocity): toward the nearest wall two blocks
        // tall within 16 blocks along +-x/+-y, at 4 blocks/s for 5 s. The game's own
        // collision stops the body; the collision log (9) records where.
        static FILE *collisionLog = nullptr; // the collision log (9)
        static bool pushKey = false;
        static ULONGLONG pushUntil = 0;
        static int pushDx = 0, pushDy = 0;
        bool down7key = GetAsyncKeyState('7') & 0x8000;
        cube::World *cw = g->world;
        auto blockSolid = [&](int64_t bx, int64_t by, int64_t bz) -> bool { // block-space z
            int zx = (int)std::floor(bx / 64.0), zy = (int)std::floor(by / 64.0);
            for (auto &[key, zone] : cw->zones)
                if (zone && zone->position.x == zx && zone->position.y == zy) {
                    const cube::Field &f = zone->fields[(bx - zx * 64) * 64 + (by - zy * 64)];
                    if (bz < f.base_z)
                        return true;
                    size_t i = (size_t)(bz - f.base_z);
                    if (i >= f.blocks.size())
                        return false;
                    uint8_t t = ((const uint8_t *)&f.blocks[i])[4];
                    return t != 0 && t != 2 && t != 3 && t != 17 && t != 19;
                }
            return false;
        };
        if (down7key && !pushKey && TryEnterCriticalSection(&cw->zones_critical_section)) {
            auto &e = cw->local_creature->entity_data;
            double px = e.position.x / 65536.0, py = e.position.y / 65536.0, pz = e.position.z / 65536.0;
            int64_t bx = (int64_t)std::floor(px), by = (int64_t)std::floor(py), feet = (int64_t)std::floor(pz) - 1;
            int best = 99;
            pushDx = pushDy = 0;
            for (auto [dx, dy] : {std::pair{1, 0}, std::pair{-1, 0}, std::pair{0, 1}, std::pair{0, -1}})
                for (int k = 1; k <= 16 && k < best; k++)
                    if (blockSolid(bx + dx * k, by + dy * k, feet) && blockSolid(bx + dx * k, by + dy * k, feet + 1)) {
                        best = k, pushDx = dx, pushDy = dy;
                        break;
                    }
            LeaveCriticalSection(&cw->zones_critical_section);
            if (best < 99) {
                pushUntil = GetTickCount64() + 5000;
                if (collisionLog)
                    fprintf(collisionLog, "# push %+d %+d toward a wall %d blocks away\n", pushDx, pushDy, best);
                log("Push toward a wall " + std::to_string(best) + " blocks away");
            } else
                log("Push: no wall within 16 blocks");
        }
        pushKey = down7key;
        if (GetTickCount64() < pushUntil) {
            auto &v = cw->local_creature->entity_data.velocity;
            if (pushDx)
                v.x = 4.f * pushDx;
            if (pushDy)
                v.y = 4.f * pushDy;
        }
        static bool collide = false, logging = false;
        bool down9 = GetAsyncKeyState('9') & 0x8000;
        if (down9 && !collide) {
            logging = !logging;
            if (logging && !collisionLog) {
                CreateDirectoryA("Extract", nullptr);
                collisionLog = _fsopen("Extract\\collision.txt", "a", _SH_DENYNO);
            }
            auto &a = g->world->local_creature->entity_data.appearance;
            if (collisionLog)
                fprintf(collisionLog, "# %s t=%llu scales graphics=%.4f hitbox=%.4f physics=%.4f\n", logging ? "start" : "stop",
                        (unsigned long long)GetTickCount64(), a.graphics_scale, a.hitbox_scale, a.physics_scale);
            log(logging ? "Collision log on" : "Collision log off");
        }
        collide = down9;
        // G: drop test (writes to the game). Puts the player 30 blocks above the nearest
        // column topped with water (in the loaded zones), so one drop logs a long fall and
        // then swimming.
        static bool dropKey = false;
        bool downG = GetAsyncKeyState('G') & 0x8000;
        if (downG && !dropKey) {
            cube::World *w = g->world;
            auto &e = w->local_creature->entity_data;
            int64_t px = e.position.x >> 16, py = e.position.y >> 16;
            int64_t best = -1, bx = 0, by = 0, bz = 0;
            EnterCriticalSection(&w->zones_critical_section); // the game streams zones meanwhile
            for (auto &[key, zone] : w->zones) {
                if (!zone)
                    continue;
                for (int lx = 0; lx < 64; lx++)
                    for (int ly = 0; ly < 64; ly++) {
                        int64_t x = (int64_t)zone->position.x * 64 + lx, y = (int64_t)zone->position.y * 64 + ly;
                        int64_t d = (x - px) * (x - px) + (y - py) * (y - py);
                        if (best >= 0 && d >= best)
                            continue;
                        // Water blocks (type 2): the topmost run of eight or more (deep enough to swim), with
                        // nothing solid above it.
                        const cube::Field &f = zone->fields[lx * 64 + ly];
                        size_t n = f.blocks.size(), run = 0;
                        int64_t top = 0;
                        for (size_t i = n; i-- > 0;) {
                            uint8_t t = ((const uint8_t *)&f.blocks[i])[4];
                            if (t == 2) {
                                if (!run)
                                    top = f.base_z + (int64_t)i + 1;
                                if (++run >= 8)
                                    break;
                            } else if (t != 0)
                                break; // solid above any water
                            else
                                run = 0;
                        }
                        if (run < 8)
                            continue;
                        best = d, bx = x, by = y, bz = top;
                    }
            }
            if (best < 0) {
                // What the loaded zones hold instead: block types, with the z range of each.
                std::map<int, std::pair<int64_t, int64_t>> range;
                std::map<int, size_t> count;
                for (auto &[key, zone] : w->zones)
                    if (zone)
                        for (int c = 0; c < 4096; c++) {
                            const cube::Field &f = zone->fields[c];
                            for (size_t i = 0; i < f.blocks.size(); i++) {
                                int t = ((const uint8_t *)&f.blocks[i])[4];
                                int64_t z = f.base_z + (int64_t)i;
                                auto r = range.try_emplace(t, z, z).first;
                                r->second.first = std::min(r->second.first, z), r->second.second = std::max(r->second.second, z);
                                count[t]++;
                            }
                        }
                std::string s;
                for (auto &[t, r] : range)
                    s += " type " + std::to_string(t) + ": " + std::to_string(count[t]) + " at z " + std::to_string(r.first) +
                         ".." + std::to_string(r.second) + ";";
                log("Drop test: no water in the " + std::to_string(w->zones.size()) + " loaded zones. Blocks:" + s +
                    " player z " + std::to_string(e.position.z >> 16));
            } else {
                e.position.x = (bx << 16) + 32768;
                e.position.y = (by << 16) + 32768;
                e.position.z = (bz + 30) << 16;
                e.velocity = FloatVector3(0, 0, 0);
                if (collisionLog)
                    fprintf(collisionLog, "# drop over water: column %lld %lld, water surface z=%lld\n", (long long)bx,
                            (long long)by, (long long)bz);
                log("Drop test: 30 blocks above water at " + std::to_string(bx) + ", " + std::to_string(by) + " (surface " +
                    std::to_string(bz) + "), " + std::to_string((int)std::sqrt((double)best)) + " blocks away");
            }
            LeaveCriticalSection(&w->zones_critical_section);
        }
        dropKey = downG;
        if (logging && collisionLog) {
            static LongVector3 last{};
            cube::World *w = g->world;
            auto &e = w->local_creature->entity_data;
            if (e.position.x != last.x || e.position.y != last.y || e.position.z != last.z) {
                // The game's own blocks (zone columns): solid unless air or liquid.
                auto solid = [&](int64_t bx, int64_t by, int64_t bz) -> bool {
                    int zx = (int)std::floor(bx / 64.0), zy = (int)std::floor(by / 64.0);
                    for (auto &[key, zone] : w->zones)
                        if (zone && zone->position.x == zx && zone->position.y == zy) {
                            const cube::Field &f = zone->fields[(bx - zx * 64) * 64 + (by - zy * 64)];
                            if (bz < f.base_z)
                                return true;
                            size_t i = (size_t)(bz - f.base_z);
                            if (i >= f.blocks.size())
                                return false;
                            uint8_t t = ((const uint8_t *)&f.blocks[i])[4];
                            return t != 0 && t != 2 && t != 3 && t != 17 && t != 19;
                        }
                    return false;
                };
                double px = e.position.x / 65536.0, py = e.position.y / 65536.0, pz = e.position.z / 65536.0;
                int64_t bx = (int64_t)std::floor(px), by = (int64_t)std::floor(py), bz = (int64_t)std::floor(pz);
                // Distance from the centre to the nearest solid face, within 3 blocks
                // (9 = none): sideways at the levels z+0.5 and z+1.5, then above and below.
                // Per block level relative to floor(z): -1, 0, +1, +2 (whether position z and
                // block z share an origin is part of what this measures).
                auto sideAt = [&](int dx, int dy, int level) {
                    for (int k = 1; k <= 3; k++)
                        if (solid(bx + dx * k, by + dy * k, bz + level)) {
                            double face = dx > 0 ? bx + k : dx < 0 ? bx - k + 1 : dy > 0 ? by + k : by - k + 1;
                            return std::abs(face - (dx ? px : py));
                        }
                    return 9.0;
                };
                std::string levels;
                auto side = [&](int dx, int dy) { return std::min(sideAt(dx, dy, 0), sideAt(dx, dy, 1)); };
                double up = 9, down = 9, gx1 = 9, gx0 = 9, gy1 = 9, gy0 = 9;
                if (TryEnterCriticalSection(&w->zones_critical_section)) {
                    gx1 = side(1, 0), gx0 = side(-1, 0), gy1 = side(0, 1), gy0 = side(0, -1);
                    for (auto [dx, dy, name] : {std::tuple{1, 0, "+x"}, std::tuple{-1, 0, "-x"}, std::tuple{0, 1, "+y"}, std::tuple{0, -1, "-y"}}) {
                        char part[96];
                        snprintf(part, sizeof part, " %s[%.3f,%.3f,%.3f,%.3f]", name, sideAt(dx, dy, -1), sideAt(dx, dy, 0),
                                 sideAt(dx, dy, 1), sideAt(dx, dy, 2));
                        levels += part;
                    }
                    for (int k = 0; k <= 4 && up == 9; k++)
                        if (solid(bx, by, bz + k + 1))
                            up = bz + k + 1 - pz;
                    for (int k = 0; k <= 4 && down == 9; k++)
                        if (solid(bx, by, bz - k))
                            down = pz - (bz - k + 1);
                    LeaveCriticalSection(&w->zones_critical_section);
                }
                fprintf(collisionLog,
                        "t=%llu pos=%.5f %.5f %.5f vel=%.3f %.3f %.3f flags=%08x gaps +x=%.4f -x=%.4f +y=%.4f -y=%.4f up=%.4f down=%.4f levels%s\n",
                        (unsigned long long)GetTickCount64(), px, py, pz, e.velocity.x, e.velocity.y, e.velocity.z, e.flags,
                        gx1, gx0, gy1, gy0, up, down, levels.c_str());
                last = e.position;
            }
        }
        static bool slay = false; // Z: fight mode's target to 0 HP (the game then kills it and drops loot its own way)
        bool downZ = GetAsyncKeyState('Z') & 0x8000;
        if (downZ && !slay && combat::targetId) {
            for (cube::World *world : {g->world, &g->host.world})
                for (cube::Creature *c : world->creatures)
                    if (c && c->id == combat::targetId && c->entity_data.HP > 0)
                        c->entity_data.HP = 0;
            log("Target set to 0 HP");
        }
        slay = downZ;
        static bool weakest = false; // F: next to the weakest monster the game simulates (fight mode's target)
        bool downF = GetAsyncKeyState('F') & 0x8000;
        if (downF && !weakest) {
            auto maxHp = (float (*)(cube::Creature *))(base() + MAX_HP_RVA);
            cube::Creature *best = nullptr;
            for (cube::Creature *c : g->world->creatures)
                if (c && c != g->world->local_creature && c->entity_data.hostility_type == 1 && c->entity_data.HP > 0 &&
                    (!best || maxHp(c) < maxHp(best)))
                    best = c;
            if (best) {
                auto &p = g->world->local_creature->entity_data;
                p.position = best->entity_data.position;
                p.position.x += 3 << 16;
                p.velocity = FloatVector3(0, 0, 0);
                combat::targetId = best->id;
                log("Weakest monster: race " + std::to_string(best->entity_data.race) + ", " +
                    std::to_string(best->entity_data.level) + " stars, max HP " + std::to_string(maxHp(best)));
            } else
                log("Weakest monster: none simulated");
        }
        weakest = downF;
        static bool arm = false; // U: the player takes fight mode's target's weapons (recording only)
        bool downU = GetAsyncKeyState('U') & 0x8000;
        if (downU && !arm && combat::targetId) {
            cube::Creature *player = g->world->local_creature;
            for (cube::Creature *c : g->world->creatures)
                if (c && c->id == combat::targetId) {
                    std::string before = statsText(player);
                    player->entity_data.equipment.weapon_right = c->entity_data.equipment.weapon_right;
                    player->entity_data.equipment.weapon_left = c->entity_data.equipment.weapon_left;
                    log("Armed with the target's weapons: " + before + " -> " + statsText(player));
                }
        }
        arm = downU;
        static bool levelKey = false; // L: the player's level from [Record] Level (recording only)
        bool downL = GetAsyncKeyState('L') & 0x8000;
        if (downL && !levelKey) {
            cube::Creature *player = g->world->local_creature;
            std::string before = statsText(player);
            player->entity_data.level = GetPrivateProfileIntA("Record", "Level", 1, ".\\Mods\\ZoneExtractor.ini");
            log("Level " + std::to_string(player->entity_data.level) + ": " + before + " -> " + statsText(player));
        }
        levelKey = downL;
        static bool turn = false; // J: the camera (and H's direction) a quarter turn
        bool downJ = GetAsyncKeyState('J') & 0x8000;
        if (downJ && !turn)
            g->camera_angle.z = g->target_camera_angle.z = g->camera_angle.z + 90;
        turn = downJ;
        uicapture::install();
        static bool f10 = false;
        bool down10 = GetAsyncKeyState(VK_F10) & 0x8000;
        if (down10 && !f10) {
            uicapture::folder = "Extract\\ui", uicapture::spacing = 300, uicapture::pending = 6;
            log("Interface capture: saving 6 frames");
        }
        f10 = down10;
    }
    // F3: creature tour. Stops around home, CREATURE_STOP_MS each, recording
    // every creature and NPC the game spawns; then home.
    {
        static bool f3 = false;
        static int creatureStop = -1;
        static ULONGLONG stopStart = 0;
        static std::vector<std::pair<int, int>> creatureStops;
        static int64_t tourX = 0, tourY = 0; // blocks: the centre of the tour
        bool down3 = GetAsyncKeyState(VK_F3) & 0x8000;
        cube::Creature *player = g->world->local_creature;
        if (down3 && !f3) {
            if (!character::creatureMode) {
                if (session != g->world)
                    startSession(g->world, player);
                // The tour goes around the extraction's home (manifest.txt, the first
                // session's home), which CubeVulkan plays, wherever this character lives.
                tourX = home.x >> 16, tourY = home.y >> 16;
                {
                    FILE *f = nullptr;
                    if (fopen_s(&f, (folder + "\\manifest.txt").c_str(), "r") == 0 && f) {
                        char line[64];
                        long long v;
                        while (fgets(line, sizeof line, f))
                            if (sscanf_s(line, "homeX=%lld", &v) == 1)
                                tourX = v;
                            else if (sscanf_s(line, "homeY=%lld", &v) == 1)
                                tourY = v;
                        fclose(f);
                    }
                }
                log("Creature tour around " + std::to_string(tourX) + ", " + std::to_string(tourY) + " (the extraction's home)");
                // Land stops only: zone centres where most of five samples of the
                // game's own ground height are above the sea, nearest first.
                creatureStops.clear();
                creatureStops.push_back({0, 0});
                int64_t hx = floorDiv(tourX, 64), hy = floorDiv(tourY, 64);
                auto ground = [&](int64_t x, int64_t y) {
                    return ((float (*)(cube::World *, int, int, int))(base() + GROUND_HEIGHT_RVA))(g->world, (int)x,
                                                                                                    (int)y, 0);
                };
                std::vector<std::pair<int, std::pair<int, int>>> land;
                int R = character::CREATURE_TOUR_RADIUS;
                for (int j = -R; j <= R; j++)
                    for (int i = -R; i <= R; i++) {
                        if (!i && !j)
                            continue;
                        int zx = i * character::CREATURE_TOUR_STEP, zy = j * character::CREATURE_TOUR_STEP;
                        int64_t cx = (hx + zx) * 64 + 32, cy = (hy + zy) * 64 + 32;
                        int dry = 0;
                        for (auto [dx, dy] : {std::pair{0, 0}, {-48, 0}, {48, 0}, {0, -48}, {0, 48}}) {
                            float h = ground(cx + dx, cy + dy);
                            dry += std::isfinite(h) && h > 2;
                        }
                        if (dry >= 3)
                            land.push_back({i * i + j * j, {zx, zy}});
                    }
                std::sort(land.begin(), land.end());
                for (auto &l : land)
                    if (creatureStops.size() < character::CREATURE_TOUR_STOPS)
                        creatureStops.push_back(l.second);
                log("Creature tour: " + std::to_string(creatureStops.size()) + " land stops (of " +
                    std::to_string((2 * R + 1) * (2 * R + 1)) + " candidates)");
                creatureStop = 0;
                stopStart = GetTickCount64();
                character::setCreatureMode(true, g->world->seed);
            } else {
                creatureStop = -1;
                character::setCreatureMode(false, g->world->seed);
                player->entity_data.position = home;
            }
        }
        f3 = down3;
        if (character::creatureMode && creatureStop >= 0) {
            if (creatureStop >= (int)creatureStops.size()) {
                creatureStop = -1;
                character::setCreatureMode(false, g->world->seed);
                player->entity_data.position = home;
                player->entity_data.velocity = FloatVector3(0, 0, 0);
                log("Creature tour finished; returned home");
            } else {
                int64_t hx = floorDiv(tourX, 64), hy = floorDiv(tourY, 64);
                int64_t zx = hx + creatureStops[creatureStop].first, zy = hy + creatureStops[creatureStop].second;
                if (creatureStop == 0) // the home itself
                    hold(g->world, player, tourX, tourY);
                else
                    hold(g->world, player, zx * 64 + 32, zy * 64 + 32);
                // The game only poses creatures it draws: turn the camera once around
                // during each stop, pulled back, so everything nearby is drawn.
                double turn = double(GetTickCount64() - stopStart) / character::CREATURE_STOP_MS * 360.0;
                g->camera_angle.z = g->target_camera_angle.z = turn;
                // Pitched toward the horizon and pulled back (Mods\ZoneExtractor.ini [Tour]
                // Pitch, Distance; read live), so creatures around the stop are on screen.
                g->camera_angle.x = g->target_camera_angle.x =
                    (double)GetPrivateProfileIntA("Tour", "Pitch", 70, ".\\Mods\\ZoneExtractor.ini");
                g->camera_distance = g->target_camera_distance =
                    (float)GetPrivateProfileIntA("Tour", "Distance", 14, ".\\Mods\\ZoneExtractor.ini");
                if (GetTickCount64() - stopStart > character::CREATURE_STOP_MS) {
                    {
                        char d[200];
                        sprintf_s(d, "  present %zu, posed %zu, named %zu; camera angle %.1f %.1f %.1f, distance %.1f",
                                  character::seenPresent, character::seenPosed, character::seenNamed,
                                  g->camera_angle.x, g->camera_angle.y, g->camera_angle.z, g->camera_distance);
                        log(d);
                    }
                    log("Creature stop " + std::to_string(creatureStop + 1) + "/" + std::to_string(creatureStops.size()) +
                        ", " + std::to_string(character::finished.size() + character::tracked.size()) + " creatures so far");
                    creatureStop++;
                    stopStart = GetTickCount64();
                }
            }
        }
        character::recordCreatures(g->world);
    }
    cube::World *w = g->world;
    cube::Creature *player = w->local_creature;
    if (session != w || sessionSeed != w->seed)
        startSession(w, player);
    writeSpriteTable(w);
    saveZones(w);
    calibration::install(w);
    static ULONGLONG nextRefresh = 0, nextWrite = 0;
    ULONGLONG now = GetTickCount64();
    if (now >= nextRefresh) {
        calibration::refresh(w);
        nextRefresh = now + 2000;
    }
    if (now >= nextWrite) {
        std::vector<std::pair<void *, std::string>> names;
        {
            std::lock_guard lock(namesMutex);
            names = spriteNames;
        }
        calibration::write(names, w);
        nextWrite = now + 10000;
    }

    static bool f11 = false, f9 = false;
    bool down11 = GetAsyncKeyState(VK_F11) & 0x8000, down9 = GetAsyncKeyState(VK_F9) & 0x8000;
    if (down11 && !f11) {
        sweeping = !sweeping;
        if (sweeping) {
            buildStops();
            stop = 0;
            stopStarted = GetTickCount64();
            log("Sweep started: " + std::to_string(stops.size()) + " stops");
        } else
            log("Sweep stopped at stop " + std::to_string(stop));
    }
    if ((down9 && !f9) || (sweeping && stop >= (int)stops.size())) {
        if (sweeping)
            log("Sweep finished; " + std::to_string(saved.size()) + " zones extracted");
        sweeping = false;
        player->entity_data.position = home;
        player->entity_data.velocity = FloatVector3(0, 0, 0);
        log("Returned home");
    }
    f11 = down11;
    f9 = down9;
    static bool f6 = false;
    bool down6 = GetAsyncKeyState(VK_F6) & 0x8000;
    if (down6 && !f6 && !sweeping) {
        touring = !touring;
        if (touring) {
            buildTour(w);
            tourIndex = 0;
            tourSide = 0;
            tourSideStarted = GetTickCount64();
            log("Calibration tour started: " + std::to_string(tour.size()) + " unmeasured building-part types");
        } else
            log("Calibration tour stopped");
    }
    f6 = down6;
    if (touring)
        tourTick(w, player);
    if (!sweeping)
        return;

    int64_t homeZx = floorDiv(home.x >> 16, 64), homeZy = floorDiv(home.y >> 16, 64);
    int zx = (int)(homeZx + stops[stop].first), zy = (int)(homeZy + stops[stop].second);
    hold(w, player, (int64_t)zx * 64 + 32, (int64_t)zy * 64 + 32);
    // Move on once the zones around this stop are saved (or after a timeout).
    bool done = true;
    for (int j = -SETTLE_RADIUS; j <= SETTLE_RADIUS && done; j++)
        for (int i = -SETTLE_RADIUS; i <= SETTLE_RADIUS && done; i++)
            done = saved.count({zx + i, zy + j}) > 0;
    if (done || GetTickCount64() - stopStarted > STOP_TIMEOUT_MS) {
        log("Stop " + std::to_string(stop + 1) + "/" + std::to_string(stops.size()) + (done ? " done" : " timed out") +
            ", " + std::to_string(saved.size()) + " zones");
        stop++;
        stopStarted = GetTickCount64();
    }
}
} // namespace extract

class ZoneExtractor : public GenericMod {
  public:
    void Initialize() override {
        std::thread(extract::writer).detach();
        extract::log("ZoneExtractor loaded (F11 sweep around home, F6 calibration tour, F9 home, F5 character snapshot, F4 record animation)");
    }
    void OnGameTick(cube::Game *g) override { extract::tick(g); }
};
EXPORT GenericMod *MakeMod() { return new ZoneExtractor(); }
// Earliest entry point: the hook must be in place before the game loads sprites.
EXPORT void ModPreInitialize() {
    extract::hookSpriteLoading();
    extract::hitprobe::install();
    extract::blockprobe::install();
    extract::chatlog::install();
    extract::animprobe::install();
    extract::updateprobe::install();
    extract::effectoracle::install();
    extract::droporacle::install();
    extract::controloracle::install();
    extract::aiprobe::install();
    extract::netrecorder::install();
}
EXPORT int ModMajorVersion() { return 7; }
EXPORT int ModMinorVersion() { return 3; }
