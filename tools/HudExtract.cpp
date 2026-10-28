// HudExtract: cuts the game's own HUD art out of frames ZoneExtractor captured
// from the running Steam game (F10, Extract/ui/frame-<n>.bmp, 1920x1080), for
// CubeVulkan's interface. Usage: HudExtract <Extract/ui folder> <frame number>
//
// For each bar (HP, MP, XP) it finds the frame by its outline, then writes:
//   hud/<bar>.png   the frame and label (the game draws them in pure black and
//                   white, so those pixels are lifted as they are; the rest is
//                   transparent),
//   hud/hud.txt     the bar's rectangles on a 1920x1080 screen, the fill colour
//                   of every row (HP), the darkening of the empty part, and the
//                   value text's colours.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct Image {
    int w = 0, h = 0;
    std::vector<uint8_t> rgb; // top-down
    const uint8_t *at(int x, int y) const { return &rgb[((size_t)y * w + x) * 3]; }
};

static bool readBmp(const fs::path &p, Image &img) {
    std::ifstream in(p, std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    if (b.size() < 54 || b[0] != 'B' || b[1] != 'M')
        return false;
    uint32_t offset, w, h;
    uint16_t bpp;
    std::memcpy(&offset, &b[10], 4);
    std::memcpy(&w, &b[18], 4);
    std::memcpy(&h, &b[22], 4);
    std::memcpy(&bpp, &b[28], 2);
    if (bpp != 24)
        return false;
    img.w = (int)w;
    img.h = (int)h;
    img.rgb.resize((size_t)w * h * 3);
    size_t row = (w * 3 + 3) & ~3u;
    for (uint32_t y = 0; y < h; y++)
        for (uint32_t x = 0; x < w; x++) {
            const uint8_t *s = &b[offset + (h - 1 - y) * row + x * 3];
            uint8_t *d = &img.rgb[((size_t)y * w + x) * 3];
            d[0] = s[2];
            d[1] = s[1];
            d[2] = s[0];
        }
    return true;
}

static bool black(const uint8_t *p) { // includes the frame's dark grey edge (64)
    return p[0] < 70 && p[1] < 70 && p[2] < 70 && std::abs(p[0] - p[2]) < 12;
}
static bool white(const uint8_t *p) { // includes the frame's grey edge (207)
    return p[0] > 200 && p[1] > 200 && p[2] > 200 && std::abs(p[0] - p[2]) < 12;
}
static bool line(const uint8_t *p) { return black(p) || white(p); }

struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
};

// From `start` stepping by (dx, dy): skips to the first black pixel, then past
// three black/white/black runs; returns where the interior begins.
static int interiorEdge(const Image &img, int x, int y, int dx, int dy, int limit) {
    int steps = 0;
    while (steps < limit && !black(img.at(x, y)))
        x += dx, y += dy, steps++;
    for (int run = 0; run < 3; run++) {
        bool wantBlack = run != 1;
        while (steps < limit && (wantBlack ? black(img.at(x, y)) : white(img.at(x, y))))
            x += dx, y += dy, steps++;
    }
    return dx ? x : y;
}

// --- Inventory screen ----------------------------------------------------------------
// `HudExtract <folder> <frame> inventory` cuts the game's inventory screen from a
// frame captured with it open (equipment tab), at the positions measured in
// that frame: the tab bar (opaque game art, icons included), the item-list
// panel and an equipment slot (their black and white frame lines; the insides
// are translucent), the darkening behind them and the text colours.
static void crop(const Image &img, Rect r, bool frameOnly, int band, const fs::path &out) {
    std::vector<uint8_t> rgba((size_t)r.w * r.h * 4, 0);
    for (int y = 0; y < r.h; y++)
        for (int x = 0; x < r.w; x++) {
            const uint8_t *p = img.at(r.x + x, r.y + y);
            int edge = std::min({x, y, r.w - 1 - x, r.h - 1 - y});
            if (frameOnly && !(line(p) && edge < band))
                continue;
            uint8_t *d = &rgba[((size_t)y * r.w + x) * 4];
            d[0] = p[0], d[1] = p[1], d[2] = p[2], d[3] = 255;
        }
    stbi_write_png(out.string().c_str(), r.w, r.h, 4, rgba.data(), r.w * 4);
}

// Darkening of a translucent panel: inside vs just outside its frame, per row
// (a black overlay of this alpha), averaged.
static float darkening(const Image &img, int insideX, int outsideX, int y0, int y1) {
    double sum = 0;
    int n = 0;
    for (int y = y0; y < y1; y++) {
        const uint8_t *in = img.at(insideX, y), *out = img.at(outsideX, y);
        for (int c = 0; c < 3; c++)
            if (out[c] > 40) {
                sum += 1.0 - double(in[c]) / out[c];
                n++;
            }
    }
    return n ? (float)std::clamp(sum / n, 0.0, 1.0) : .5f;
}

// Most common bright colour in a rectangle (the text colour there).
static std::string textColour(const Image &img, Rect r) {
    std::map<uint32_t, int> count;
    for (int y = r.y; y < r.y + r.h; y++)
        for (int x = r.x; x < r.x + r.w; x++) {
            const uint8_t *p = img.at(x, y);
            if (p[0] + p[1] + p[2] > 300)
                count[(uint32_t)p[0] << 16 | p[1] << 8 | p[2]]++;
        }
    uint32_t best = 0xffffff;
    int most = 0;
    for (auto &[c, n] : count)
        if (n > most)
            most = n, best = c;
    return std::to_string(best >> 16) + " " + std::to_string((best >> 8) & 255) + " " + std::to_string(best & 255);
}

static int inventory(const Image &img, const fs::path &hud, const char *frame) {
    // Positions measured in the captured frame (1920x1080).
    const Rect tabs{712, 44, 410, 72}, panel{704, 107, 512, 376}, slot{705, 555, 72, 72};
    crop(img, tabs, false, 0, hud / "tabs.png");
    crop(img, panel, true, 10, hud / "panel.png");
    crop(img, slot, true, 9, hud / "slot.png");
    std::ofstream txt(hud / "inventory.txt");
    txt << "# Cut from the game's own frame " << frame << " (1920x1080) by tools/HudExtract (inventory screen).\n";
    txt << "tabs " << tabs.x << " " << tabs.y << " " << tabs.w << " " << tabs.h << "\n";
    txt << "panel " << panel.x << " " << panel.y << " " << panel.w << " " << panel.h << " darkening "
        << darkening(img, 718, 700, 200, 440) << "\n";
    // Ten slots: two columns, rows 70 px apart (left: weapon, ring, neck, chest, feet;
    // right: weapon, ring, shoulder, hands, pet).
    txt << "slots left 705 right 1143 top 485 pitch 70 size 72 darkening " << darkening(img, 760, 700, 640, 680)
        << "\n";
    txt << "colour header " << textColour(img, {725, 132, 145, 18}) << "\n";
    txt << "colour item " << textColour(img, {785, 166, 115, 16}) << "\n";
    txt << "colour label " << textColour(img, {783, 585, 105, 15}) << "\n";
    txt << "colour name " << textColour(img, {22, 198, 190, 18}) << "\n";
    txt << "colour level " << textColour(img, {22, 228, 110, 18}) << "\n";
    txt << "colour stat " << textColour(img, {22, 356, 140, 14}) << "\n";
    txt << "colour value " << textColour(img, {322, 356, 40, 14}) << "\n";
    txt << "colour skill " << textColour(img, {22, 526, 170, 14}) << "\n";
    txt << "colour region " << textColour(img, {860, 450, 120, 16}) << "\n";
    std::printf("inventory: tabs, panel and slot cut; see inventory.txt\n");
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::printf("usage: HudExtract <Extract/ui folder> <frame number>\n");
        return 1;
    }
    fs::path dir = argv[1];
    Image img;
    if (!readBmp(dir / ("frame-" + std::string(argv[2]) + ".bmp"), img) || img.w != 1920 || img.h != 1080) {
        std::printf("need a 1920x1080 24-bit frame\n");
        return 1;
    }
    fs::create_directories(dir / "hud");
    if (argc >= 4 && std::string(argv[3]) == "inventory")
        return inventory(img, dir / "hud", argv[2]);
    std::ofstream txt(dir / "hud" / "hud.txt");
    txt << "# Cut from the game's own frame " << argv[2] << " (1920x1080) by tools/HudExtract.\n";
    // Search boxes around where the game draws each bar at 1920x1080.
    struct Bar {
        const char *name;
        Rect search;
    } bars[] = {{"hp", {590, 950, 374, 100}}, {"mp", {965, 950, 370, 100}}, {"xp", {555, 850, 815, 100}}};
    for (auto &bar : bars) {
        const Rect &s = bar.search;
        int midY = s.y + s.h * 2 / 3, midX = s.x + s.w / 4; // left of the value text
        Rect in;
        in.x = interiorEdge(img, s.x, midY, 1, 0, s.w);
        int right = interiorEdge(img, s.x + s.w - 1, midY, -1, 0, s.w);
        in.y = interiorEdge(img, midX, s.y, 0, 1, s.h);
        int bottom = interiorEdge(img, midX, s.y + s.h - 1, 0, -1, s.h);
        in.w = right - in.x + 1;
        in.h = bottom - in.y + 1;
        // Outer box: black/white pixels connected around the interior (label tab included).
        Rect out{in.x - 16, in.y - 30, in.w + 32, in.h + 48};
        out.x = std::max(out.x, 0);
        out.y = std::max(out.y, 0);
        std::vector<uint8_t> rgba((size_t)out.w * out.h * 4, 0);
        for (int y = 0; y < out.h; y++)
            for (int x = 0; x < out.w; x++) {
                int gx = out.x + x, gy = out.y + y;
                bool inside = gx >= in.x && gx < in.x + in.w && gy >= in.y && gy < in.y + in.h;
                const uint8_t *p = img.at(gx, gy);
                // Only the border (16 px around the interior) and the label tab at the top left.
                int ox = std::max({in.x - gx, gx - (in.x + in.w - 1), 0}), oy = std::max({in.y - gy, gy - (in.y + in.h - 1), 0});
                bool border = ox <= 16 && oy <= 16, label = gx < in.x + 70 && gy < in.y;
                if (!inside && (border || label) && line(p)) {
                    uint8_t *d = &rgba[((size_t)y * out.w + x) * 4];
                    d[0] = p[0], d[1] = p[1], d[2] = p[2], d[3] = 255;
                }
            }
        stbi_write_png((dir / "hud" / (std::string(bar.name) + ".png")).string().c_str(), out.w, out.h, 4,
                       rgba.data(), out.w * 4);
        txt << "bar " << bar.name << " frame " << out.x << " " << out.y << " " << out.w << " " << out.h
            << " interior " << in.x << " " << in.y << " " << in.w << " " << in.h << "\n";
        std::printf("%s: frame %d,%d %dx%d interior %d,%d %dx%d\n", bar.name, out.x, out.y, out.w, out.h, in.x,
                    in.y, in.w, in.h);
        // Fill: the colour of each interior row at its left end (left of the text).
        for (int y = 0; y < in.h; y++) {
            const uint8_t *p = img.at(in.x + 12, in.y + y);
            txt << "fill " << bar.name << " " << y << " " << (int)p[0] << " " << (int)p[1] << " " << (int)p[2] << "\n";
        }
        // Empty part: the darkening of each row at the interior's right end,
        // relative to the world just above the bar (black overlay, alpha per row).
        const uint8_t *sky = img.at(in.x + in.w - 12, out.y - 4);
        for (int y = 0; y < in.h; y++) {
            const uint8_t *p = img.at(in.x + in.w - 12, in.y + y);
            double a = 0;
            int n = 0;
            for (int c = 0; c < 3; c++)
                if (sky[c] > 30) {
                    a += 1.0 - double(p[c]) / sky[c];
                    n++;
                }
            a = n ? std::clamp(a / n, 0.0, 1.0) : .5;
            txt << "empty " << bar.name << " " << y << " " << (int)p[0] << " " << (int)p[1] << " " << (int)p[2] << " "
                << a << "\n";
        }
        // Value text: darkest and brightest colours in the middle third of the
        // interior that differ from that row's fill.
        int bestDark = 999, bestLight = -1;
        const uint8_t *dark = nullptr, *light = nullptr;
        for (int y = in.y + 2; y < in.y + in.h - 2; y++)
            for (int x = in.x + in.w / 3; x < in.x + 2 * in.w / 3; x++) {
                const uint8_t *p = img.at(x, y), *f = img.at(in.x + 12, y);
                if (std::abs(p[0] - f[0]) + std::abs(p[1] - f[1]) + std::abs(p[2] - f[2]) < 40)
                    continue;
                int l = p[0] + p[1] + p[2];
                if (l < bestDark)
                    bestDark = l, dark = p;
                if (l > bestLight)
                    bestLight = l, light = p;
            }
        if (dark && light)
            txt << "text " << bar.name << " " << (int)light[0] << " " << (int)light[1] << " " << (int)light[2] << " "
                << (int)dark[0] << " " << (int)dark[1] << " " << (int)dark[2] << "\n";
    }
    return 0;
}
