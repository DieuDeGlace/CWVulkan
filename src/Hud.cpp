#include "Hud.h"
#include "IdentifyWindow.h"
#include "FeatText.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"
#include <glm/gtc/matrix_transform.hpp>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <functional>
#include <sstream>

namespace fs = std::filesystem;

namespace {
std::vector<unsigned char> readFile(const fs::path &p) {
    std::ifstream in(p, std::ios::binary);
    return std::vector<unsigned char>((std::istreambuf_iterator<char>(in)), {});
}

// Sizes measured in the game's own frames at 1920x1080 (digit heights).
constexpr float VALUE_DIGITS = 10, DAMAGE_DIGITS = 16, PLATE_DIGITS = 10;
// Monster health bar over the head, measured in a captured fight frame.
constexpr float PLATE_W = 112, PLATE_H = 16;
// Health fills from the left; the part just lost flashes bright and shrinks away.
const glm::u8vec4 PLATE_FILL{184, 28, 56, 255}, PLATE_TRAIL{248, 82, 45, 255}, PLATE_EMPTY{0, 0, 0, 100},
    PLATE_TOP{255, 179, 179, 255}, PLATE_BOTTOM{255, 74, 74, 255}, BLACK{0, 0, 0, 255};
// Damage numbers: the game's red with its dark outline, as captured.
const glm::u8vec4 DAMAGE_FILL{203, 64, 75, 255}, DAMAGE_OUTLINE{38, 14, 25, 255};

// Simple shelf packer for the atlas.
struct Packer {
    int width, x = 0, y = 0, shelf = 0;
    bool full = false; // something did not fit (it is put at the corner; the atlas is redone larger)
    explicit Packer(int w) : width(w) {}
    glm::ivec2 place(int w, int h) {
        if (x + w > width) {
            x = 0;
            y += shelf + 1;
            shelf = 0;
        }
        if (y + h > width || w > width) {
            full = true;
            return glm::ivec2(0);
        }
        glm::ivec2 at(x, y);
        x += w + 1;
        shelf = std::max(shelf, h);
        return at;
    }
};
} // namespace

bool Hud::loadMenu(const fs::path &gameDir, std::string &error) {
    error.clear();
    hudFont = readFile(gameDir / "resource1.dat");
    titleFont = readFile(gameDir / "resource2.dat");
    stbtt_fontinfo probe;
    if (hudFont.empty() || !stbtt_InitFont(&probe, hudFont.data(), 0)) {
        error = "cannot read the game's font resource1.dat";
        return false;
    }
    if (titleFont.empty() || !stbtt_InitFont(&probe, titleFont.data(), 0))
        titleFont = hudFont;
    return true;
}

bool Hud::load(const fs::path &gameDir, std::string &error) {
    if (!loadMenu(gameDir, error))
        return false;
    { // the game's interface file and scale (its options.cfg)
        std::string why;
        // (the game reads gui.plx and icons.plx with scalable text, cursor.plx without)
        const std::vector<uint64_t> keys = {plx::keyOf("PlasmaXGraphics")};
        guiLoaded = plx::read(gameDir / "gui.plx", gui, keys, &why, plx::LOAD_SCALABLE_TEXT);
        iconsLoaded = plx::read(gameDir / "icons.plx", iconsDoc, keys, &why, plx::LOAD_SCALABLE_TEXT);
        // Interface's its pointer node: a node under the engine's root holding cursor.plx's nodes (in
        // place of the file's own root and its page placement), its text empty.
        if (plx::read(gameDir / "cursor.plx", cursorDoc, keys, &why) && !cursorDoc.nodes.empty()) {
            cursorScene = std::make_unique<plasma::Scene>(cursorDoc);
            cursorNode = cursorScene->create(&cursorScene->root(), "cursor");
            for (int child : cursorDoc.nodes[0].children)
                cursorScene->instance(child, cursorNode);
            cursorNode->setAllText(L"");
        }
        std::ifstream options(gameDir / "options.cfg");
        std::string key;
        float v;
        while (options >> key >> v)
            if (key == "guiScale" && v > 0.0f)
                guiScale = v;
    }
    fs::path dir = gameDir / "Extract" / "ui" / "hud";
    std::ifstream txt(dir / "hud.txt");
    if (!txt && !guiLoaded) { // (the captured HUD art is only a fallback for a missing gui.plx)
        error = "no Extract/ui/hud (capture with ZoneExtractor F10, then run tools/HudExtract)";
        return false;
    }
    std::string line;
    while (std::getline(txt, line)) {
        std::istringstream in(line);
        std::string kind, name;
        in >> kind >> name;
        if (kind == "bar") {
            Bar &b = bars[name];
            std::string word;
            in >> word >> b.frame.x >> b.frame.y >> b.frame.z >> b.frame.w >> word >> b.interior.x >> b.interior.y >>
                b.interior.z >> b.interior.w;
        } else if (kind == "fill") {
            int row, r, g, bl;
            in >> row >> r >> g >> bl;
            bars[name].fill.push_back(glm::u8vec3(r, g, bl));
        } else if (kind == "empty") {
            int row, r, g, bl;
            float a;
            in >> row >> r >> g >> bl >> a;
            bars[name].emptyAlpha.push_back(a);
        } else if (kind == "text") {
            int r, g, bl, r2, g2, b2;
            in >> r >> g >> bl >> r2 >> g2 >> b2;
            bars[name].text = glm::u8vec3(r, g, bl);
            bars[name].outline = glm::u8vec3(r2, g2, b2);
        }
    }
    for (auto &[name, b] : bars) {
        int n;
        unsigned char *px = stbi_load((dir / (name + ".png")).string().c_str(), &b.imageW, &b.imageH, &n, 4);
        if (px) {
            b.image.assign(px, px + (size_t)b.imageW * b.imageH * 4);
            stbi_image_free(px);
        }
    }
    if ((!bars.count("hp") || bars["hp"].image.empty()) && !guiLoaded) {
        error = "Extract/ui/hud is incomplete";
        return false;
    }
    // The XP and MP bars were captured empty (no XP, no mana), so their "fill" is
    // the grey empty colour. Until frames with them filled are captured, fill them
    // with the bar's own text colour (XP: PyroProgression's purple, MP: cyan),
    // shaded row by row as the health bar's fill is.
    for (auto &[name, b] : bars) {
        if (name == "hp" || b.fill.empty() || bars["hp"].fill.empty())
            continue;
        float grey = 0;
        for (auto &c : b.fill)
            grey += std::max({c.r, c.g, c.b}) - std::min({c.r, c.g, c.b});
        if (grey / b.fill.size() > 40)
            continue; // a real, coloured fill was captured
        const auto &hp = bars["hp"].fill;
        float brightest = 1;
        for (auto &c : hp)
            brightest = std::max(brightest, (float)std::max({c.r, c.g, c.b}));
        for (size_t row = 0; row < b.fill.size(); row++) {
            const auto &c = hp[std::min(hp.size() - 1, row * hp.size() / b.fill.size())];
            float shade = std::max({c.r, c.g, c.b}) / brightest;
            b.fill[row] = glm::u8vec3(glm::vec3(b.text) * shade);
        }
    }
    // Inventory screen art, when it has been extracted too.
    {
        auto picture = [&](const char *file, Picture &p) {
            int n;
            unsigned char *px = stbi_load((dir / file).string().c_str(), &p.w, &p.h, &n, 4);
            if (!px)
                return false;
            p.rgba.assign(px, px + (size_t)p.w * p.h * 4);
            stbi_image_free(px);
            return true;
        };
        std::ifstream inv(dir / "inventory.txt");
        std::string row;
        while (std::getline(inv, row)) {
            std::istringstream in(row);
            std::string kind, name, word;
            in >> kind;
            if (kind == "colour") {
                int r, g, b;
                in >> name >> r >> g >> b;
                colours[name] = glm::u8vec3(r, g, b);
            } else if (kind == "panel") {
                int x, y, w, h;
                in >> x >> y >> w >> h >> word >> panelDark;
            } else if (kind == "slots") {
                while (in >> word)
                    if (word == "darkening")
                        in >> slotDark;
            }
        }
        inventoryReady = inv.is_open() && picture("tabs.png", tabs) && picture("panel.png", panel) &&
                         picture("slot.png", slotFrame);
    }
    // Creature names: <name id="RadishCreature" ...> { #plural Radishlings | Radishling }
    std::ifstream dict(gameDir / "dict_en.xml");
    std::string id;
    while (std::getline(dict, line)) {
        size_t at = line.find("<name id=\"");
        if (at != std::string::npos) {
            size_t end = line.find('"', at + 10);
            id = line.substr(at + 10, end - at - 10);
            continue;
        }
        size_t bar = line.find('|');
        if (!id.empty() && line.find("#plural") != std::string::npos && bar != std::string::npos) {
            size_t close = line.find('}', bar);
            std::string singular = line.substr(bar + 1, close - bar - 1);
            singular.erase(0, singular.find_first_not_of(' '));
            singular.erase(singular.find_last_not_of(' ') + 1);
            names[id] = singular;
            id.clear();
        }
    }
    buildPanels();
    buildMapHistory();
    return true;
}

std::string Hud::nameForSprite(const std::string &sprite) const {
    auto it = names.find(nameIdForSprite(sprite));
    return it == names.end() ? "" : it->second;
}

std::string Hud::nameIdForSprite(const std::string &sprite) const {
    // "radish-creature-head" -> "RadishCreature"; drop part and variant suffixes.
    std::string id;
    bool upper = true;
    std::istringstream parts(sprite);
    std::string part;
    while (std::getline(parts, part, '-')) {
        if (part == "head" || part == "body" || part == "hair" || part == "foot" || part == "hand" ||
            (part.size() >= 2 && (part[0] == 'm' || part[0] == 'f') && std::isdigit((unsigned char)part[1])) ||
            std::isdigit((unsigned char)part.back()))
            break;
        for (char c : part) {
            id += upper ? (char)std::toupper((unsigned char)c) : c;
            upper = false;
        }
        upper = true;
    }
    return names.count(id) ? id : std::string();
}

// The interface nodes drawn from gui.plx (their text shapes' glyphs are rastered with the atlas).
static const char *const PLASMA_HUD[] = {"hpbar", "mpbar", "castbar", "staminabar", "quickbutton", "crosshair", "zoomcrosshair", "location",
                                         "equipmentboxleft", "equipmentboxright", "tab", "wideitembox", "blackwidget", "preview",
                                         "enemylifebar", "friendlifebar", "staticlifebar", "neutrallifebar", "enemylifebar:small",
                                         "friendlifebar:small", "staticlifebar:small", "neutrallifebar:small",
                                         // the frontend's templates (HudFrontend.cpp)
                                         "button", "button2", "smallbutton", "edit", "leftbutton", "rightbutton"};
// The texts the interface makes in code (the interface's code-made texts): size and stroke.
static const int PLASMA_LABELS[][2] = {{24, 5}, {12, 3}, {28, 6}, {21, 5}, {16, 3}, {16, 4}, {15, 4}, {15, 3}, {18, 3}, {12, 2},
                                       {18, 4}, {13, 3}}; // (the frontend's menus: frontend-title.md, frontend-options.md)

plasma::TextStyle Hud::labelStyle(int size, int stroke, int flags, bool scalable) const {
    plasma::TextStyle s;
    s.font = "resource1.dat";
    s.pixelSize = (float)size;
    s.strokeRadius = (float)stroke;
    s.lineSpacing = 3.0f;
    s.flags = flags;
    s.scalable = scalable;
    s.color = glm::vec4(1);
    s.strokeColor = glm::vec4(0, 0, 0, 1);
    return s;
}

// The outlined text shape pairs the interface makes (plasma-shapes-and-text.md 7.2): pixel mode.
void Hud::plasmaLabel(std::vector<OverlayVertex> &out, const std::wstring &text, glm::vec2 position, glm::vec2 offset, int size,
                      int stroke, int flags) const {
    glm::mat4 m(1);
    m[0][0] = m[1][1] = guiScale;
    m[3] = glm::vec4((position / guiScale + offset) * guiScale, 0, 1);
    plasma::TextStyle s = labelStyle(size, stroke, flags, false);
    s.string = text;
    plasmaText(out, s, m);
    s.strokeRadius = 0;
    plasmaText(out, s, m);
}

template <class Pack> void Hud::bakeText(Pack &pack, std::vector<uint8_t> &atlas, int width) {
    textGlyphs.clear();
    if (!guiLoaded)
        return;
    if (!plasmaFonts[0].loaded())
        plasmaFonts[0].load(hudFont);
    if (!plasmaFonts[1].loaded())
        plasmaFonts[1].load(titleFont);
    glm::mat4 root(1);
    root[0][0] = root[1][1] = guiScale;
    // The characters the interface writes: printable ASCII, the degree sign of the temperature,
    // the coin of the prices and the rarity mark the life bars use.
    std::vector<wchar_t> characters;
    for (wchar_t c = 32; c < 127; c++)
        characters.push_back(c);
    for (wchar_t c : {0xb0, 0xae, 0xb1})
        characters.push_back(c);
    characters.insert(characters.end(), additionalGlyphs.begin(), additionalGlyphs.end());
    auto bakeStyle = [&](const plasma::TextStyle &style, const glm::mat4 &m) {
        if (style.pixelSize <= 0.0f)
            return;
        const int f = fontIndex(style.font);
        const plasma::Raster r = plasma::rasterFor(m, style);
        for (bool outline : {false, true}) {
            if (outline && r.stroke <= 0.0f)
                continue;
            for (wchar_t c : characters) {
                const auto key = std::make_tuple(f, r.key(), outline, c);
                if (textGlyphs.count(key))
                    continue;
                const plasma::Glyph &g = plasmaFonts[f].glyph(c, r, outline);
                TextGlyph t;
                t.offset = g.offset, t.extent = g.extent;
                if (g.exists && std::any_of(g.alpha.begin(), g.alpha.end(), [](uint8_t a) { return a != 0; })) {
                    t.size = g.size;
                    t.atlas = pack.place(g.size.x, g.size.y);
                    for (int y = 0; y < g.size.y; y++)
                        for (int x = 0; x < g.size.x; x++) {
                            uint8_t *d = &atlas[((size_t)(t.atlas.y + y) * width + t.atlas.x + x) * 4];
                            d[0] = d[1] = d[2] = 255;
                            d[3] = g.alpha[(size_t)y * g.size.x + x];
                        }
                }
                textGlyphs[key] = t;
            }
        }
    };
    std::function<void(const plx::Document &, int, glm::mat4)> walk = [&](const plx::Document &doc, int index, glm::mat4 m) {
        const plx::Node &n = doc.nodes[(size_t)index];
        if (n.transformation >= 0 && n.transformation < (int)doc.transformations.size())
            m = m * plasma::transformation(doc.transformations[(size_t)n.transformation]);
        plasma::TextStyle style;
        if (n.shape >= 0 && n.shape < (int)doc.shapes.size() && plasma::textStyle(doc.shapes[(size_t)n.shape], doc.loaderFlags, style))
            bakeStyle(style, m);
        for (int c : n.children)
            if (c >= 0 && c < (int)doc.nodes.size())
                walk(doc, c, m);
    };
    for (const char *name : PLASMA_HUD)
        if (const int i = gui.find(name); i >= 0)
            walk(gui, i, root);
    if (cursorNode) // (the pointer's text, unscaled)
        for (int child : cursorDoc.nodes[0].children)
            walk(cursorDoc, child, glm::mat4(1));
    // The map's labels (MapOverlayWidget): (int)(scale * 12) pixels with (int)(scale * 3) of
    // outline, the scale 0.35 to 1, unscaled by the interface.
    for (int size = 4; size <= 12; size++)
        for (int stroke = 0; stroke <= 3; stroke++)
            bakeStyle(labelStyle(size, stroke, 0, true), glm::mat4(1));
    // The interface's code-made texts: pixel mode outlined pairs, and widget texts (scalable).
    for (auto [size, stroke] : PLASMA_LABELS)
        for (int s : {stroke, 0})
            for (bool scalable : {false, true})
                bakeStyle(labelStyle(size, s, 0, scalable), root);
    // Equipped-item details scale fonts by three quarters; outline widths remain
    // unchanged. Keep those fractional styles in the shared atlas too.
    for (const glm::vec2 font : {glm::vec2(11.25f, 4), glm::vec2(11.25f, 3), glm::vec2(9, 3), glm::vec2(13.5f, 3), glm::vec2(9, 2)})
        for (float stroke : {font.y, 0.f}) {
            auto style = labelStyle(12, 0, 0, true);
            style.pixelSize = font.x;
            style.strokeRadius = stroke;
            style.lineSpacing = 1;
            bakeStyle(style, root);
        }
    plasma::TextStyle featStyle;
    featStyle.font = "resource1.dat";
    featStyle.pixelSize = feattext::FONT_SIZE;
    featStyle.strokeRadius = feattext::OUTLINE;
    bakeStyle(featStyle, glm::mat4(1));
    featStyle.strokeRadius = 0;
    bakeStyle(featStyle, glm::mat4(1));
}

void Hud::prepareFrame(Renderer &renderer, VkExtent2D screen) {
    if (atlasDirty)
        layout(renderer, screen);
}

void Hud::layout(Renderer &renderer, VkExtent2D screen) {
    uploadItemModels(renderer);
    if (hudFont.empty() || !screen.height || !screen.width)
        return;
    scale = std::min(screen.height / 1080.f, screen.width / 1280.f);
    this->screen = glm::vec2((float)screen.width, (float)screen.height);
    offsetX = (screen.width - 1920 * scale) / 2; // The game centres its HUD.
    const int W = std::max(scale >= 1.f ? 4096 : 2048, atlasSize);
    std::vector<uint8_t> atlas(W * W * 4, 0);
    Packer pack(W);
    white = pack.place(4, 4);
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            std::fill_n(&atlas[((white.y + y) * W + white.x + x) * 4], 4, 255);
    for (auto &[name, b] : bars) {
        b.atlas = pack.place(b.imageW, b.imageH);
        for (int y = 0; y < b.imageH; y++)
            std::copy_n(&b.image[(size_t)y * b.imageW * 4], b.imageW * 4, &atlas[((b.atlas.y + y) * W + b.atlas.x) * 4]);
    }
    // Glyphs of the game's HUD font, sized so digits are as tall as in the game.
    stbtt_fontinfo font;
    stbtt_InitFont(&font, hudFont.data(), 0);
    auto bake = [&](Font &out, float digits, bool smooth = false) {
        out.glyphs.clear();
        out.smoothStroke = smooth;
        int x0, y0, x1, y1;
        stbtt_GetCodepointBox(&font, '0', &x0, &y0, &x1, &y1);
        float s = digits * scale / float(y1 - y0);
        int ascent;
        stbtt_GetFontVMetrics(&font, &ascent, nullptr, nullptr);
        out.ascent = ascent * s;
        for (int c = 32; c < 127; c++) {
            Glyph g;
            int adv, lsb;
            stbtt_GetCodepointHMetrics(&font, c, &adv, &lsb);
            g.advance = adv * s;
            int bx0, by0, bx1, by1;
            stbtt_GetCodepointBitmapBox(&font, c, s, s, &bx0, &by0, &bx1, &by1);
            g.size = {bx1 - bx0, by1 - by0};
            g.offset = {bx0, by0};
            if (g.size.x > 0 && g.size.y > 0) {
                g.atlas = pack.place(g.size.x, g.size.y);
                std::vector<unsigned char> mono(g.size.x * g.size.y);
                stbtt_MakeCodepointBitmap(&font, mono.data(), g.size.x, g.size.y, g.size.x, s, s, c);
                for (int y = 0; y < g.size.y; y++)
                    for (int x = 0; x < g.size.x; x++) {
                        uint8_t *d = &atlas[((g.atlas.y + y) * W + g.atlas.x + x) * 4];
                        // Pixel font: hard edges, as the game draws it.
                        uint8_t a = smooth ? mono[y * g.size.x + x] : mono[y * g.size.x + x] >= 128 ? 255 : 0;
                        d[0] = d[1] = d[2] = 255;
                        d[3] = a;
                    }
                if (smooth) {
                    // A continuous round stroke, as native vector text uses.
                    // Eight shifted glyphs leave holes when the outline is wide.
                    for (int stroke = 0; stroke < 2; stroke++) {
                        int radius = std::max(1, (int)std::round((stroke ? 5 : 3) * scale));
                        g.strokeRadius[stroke] = radius;
                        int w = g.size.x + radius * 2, h = g.size.y + radius * 2;
                        g.strokeAtlas[stroke] = pack.place(w, h);
                        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
                            uint8_t alpha = 0;
                            for (int dy = -radius; dy <= radius; dy++) for (int dx = -radius; dx <= radius; dx++) {
                                int sx = x + dx - radius, sy = y + dy - radius;
                                if (dx * dx + dy * dy <= radius * radius && sx >= 0 && sy >= 0 &&
                                    sx < g.size.x && sy < g.size.y)
                                    alpha = std::max(alpha, mono[sy * g.size.x + sx]);
                            }
                            auto at = g.strokeAtlas[stroke];
                            uint8_t *d = &atlas[((at.y + y) * W + at.x + x) * 4];
                            d[0] = d[1] = d[2] = 255; d[3] = alpha;
                        }
                    }
                }
            }
            out.glyphs[c] = g;
        }
    };
    bake(valueFont, VALUE_DIGITS);
    bake(damageFont, DAMAGE_DIGITS);
    bake(plateFont, PLATE_DIGITS);
    bake(labelFont, 11); // inventory labels and values (as measured in the captured screen)
    bake(nameFont, 15); // character name and level
    stbtt_InitFont(&font, titleFont.data(), 0);
    if (inventoryReady) {
        auto put = [&](Picture &p) {
            p.atlas = pack.place(p.w, p.h);
            for (int y = 0; y < p.h; y++)
                std::copy_n(&p.rgba[(size_t)y * p.w * 4], p.w * 4, &atlas[((p.atlas.y + y) * W + p.atlas.x) * 4]);
        };
        put(tabs);
        put(panel);
        put(slotFrame);
    }
    for (auto &[key, icon] : icons) { // item icons and the menu's character portraits
        icon.atlas = pack.place(icon.w, icon.h);
        for (int y = 0; y < icon.h; y++)
            std::copy_n(&icon.rgba[(size_t)y * icon.w * 4], icon.w * 4, &atlas[((icon.atlas.y + y) * W + icon.atlas.x) * 4]);
    }
    bakeText(pack, atlas, W);
    if (pack.full && W < 8192) { // (the icons added since outgrew it)
        atlasSize = W * 2;
        layout(renderer, screen);
        return;
    }
    renderer.setOverlayAtlas(W, W, atlas.data());
    atlasDirty = false;
    ready = true;
}

// A text shape's drawing (plasma-shapes-and-text.md 6.1, 6.8): each glyph a textured quad at its
// pen through the draw matrix, the plain glyph in the text colour and, with an outline, the
// outline glyph over it in the outline colour. (The glyphs come from the atlas, baked by
// bakeText(); one that was not baked is left out.) Returns the final pen's y in text units.
float Hud::plasmaText(std::vector<OverlayVertex> &out, const plasma::TextStyle &style, const glm::mat4 &previous, glm::vec2 *pen) const {
    if (pen)
        *pen = {};
    if (style.string.empty() || style.pixelSize <= 0.0f)
        return 0.0f;
    const int f = fontIndex(style.font);
    const plasma::Font &font = plasmaFonts[f];
    if (!font.loaded())
        return 0.0f;
    const plasma::Raster r = plasma::rasterFor(previous, style);
    const glm::mat4 m = plasma::drawMatrix(previous, style, r);
    const auto rasterKey = r.key();
    auto quad = [&](const plasma::Placed &p, bool outline, const glm::vec4 &colour) {
        auto it = textGlyphs.find({f, rasterKey, outline, p.c});
        if (it == textGlyphs.end() || it->second.size.x <= 0)
            return;
        const TextGlyph &g = it->second;
        const glm::vec2 at = plasma::glyphPosition(r, p.pen, g.offset);
        const glm::vec4 c = glm::clamp(colour, glm::vec4(0), glm::vec4(1)) * 255.0f;
        OverlayVertex v[4];
        for (int k = 0; k < 4; k++) {
            const glm::vec2 corner = at + glm::vec2(k & 1 ? g.extent.x : 0.0f, k & 2 ? g.extent.y : 0.0f);
            const glm::vec4 s = m * glm::vec4(corner, 0, 1);
            v[k] = {s.x + plasma::DRAW_OFFSET, s.y + plasma::DRAW_OFFSET, (float)(g.atlas.x + (k & 1 ? g.size.x : 0)),
                    (float)(g.atlas.y + (k & 2 ? g.size.y : 0)), (uint8_t)c.r, (uint8_t)c.g, (uint8_t)c.b, (uint8_t)c.a};
        }
        out.insert(out.end(), {v[0], v[1], v[2], v[1], v[3], v[2]});
    };
    const plasma::Layout text = plasma::layout(font, style.string, r, style.flags);
    for (const plasma::Placed &p : text.glyphs) {
        quad(p, false, style.color);
        if (r.stroke > 0.0f)
            quad(p, true, style.strokeColor);
    }
    if (pen)
        *pen = text.pen * style.pixelSize / r.size;
    return text.pen.y * style.pixelSize / r.size.y;
}

// A colour with the same value and saturation in another hue (degrees).
static glm::vec4 inHue(glm::vec4 c, float hue) {
    const float hi = std::max({c.r, c.g, c.b}), lo = std::min({c.r, c.g, c.b});
    const float h = std::fmod(hue / 60.0f, 6.0f), x = (hi - lo) * (1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f));
    const glm::vec3 k = h < 1 ? glm::vec3(hi, lo + x, lo) : h < 2 ? glm::vec3(lo + x, hi, lo) : h < 3 ? glm::vec3(lo, hi, lo + x)
                      : h < 4 ? glm::vec3(lo, lo + x, hi) : h < 5 ? glm::vec3(lo + x, lo, hi) : glm::vec3(hi, lo, lo + x);
    return glm::vec4(k, c.a);
}

void Hud::plasmaNode(std::vector<OverlayVertex> &out, int index, const glm::mat4 &parent, glm::vec4 tint,
                     const PlasmaFrame &frame, const std::wstring *text, const glm::vec4 *color, const float *hue) const {
    const auto &fills = frame.fills;
    const plx::Node &n = gui.nodes[(size_t)index];
    if (auto it = frame.texts.find(n.name); it != frame.texts.end())
        text = &it->second;
    if (auto it = frame.colors.find(n.name); it != frame.colors.end())
        color = &it->second;
    if (auto it = frame.hues.find(n.name); it != frame.hues.end())
        hue = &it->second;
    if (frame.hidden.count(n.name))
        return;
    // A clipping node draws its children into a layer and then its own shape sampling that layer
    // (plasma-format-and-scene.md 2.3.1, mask modes 2.3.3). CubeVulkan has no layers in its overlay:
    // it draws the children unclipped, with the mask arithmetic applied to their colours using the
    // shape's mean colour at the playing frame (mode 2 multiplies by all of it, the other modes
    // by its opacity).
    bool clips = false;
    if (n.display >= 0 && n.display < (int)gui.displays.size() && n.shape >= 0 && n.shape < (int)gui.shapes.size()) {
        const plasma::Display display = plasma::displayOf(gui.displays[(size_t)n.display]);
        if (display.clips) {
            clips = true;
            glm::vec4 mean(1);
            if (const plx::Chunk *c = gui.shapes[(size_t)n.shape].property("SmoothMeshShape.vertexColors")) {
                std::vector<float> v;
                if (!plasma::sampleSequence(*c, frame.sequence, frame.sequenceMs, v))
                    for (glm::vec4 colour : plx::liveArray<glm::vec4>(c))
                        v.insert(v.end(), {colour.r, colour.g, colour.b, colour.a});
                if (v.size() >= 4) {
                    mean = glm::vec4(0);
                    for (size_t k = 0; k + 3 < v.size(); k += 4)
                        mean += glm::vec4(v[k], v[k + 1], v[k + 2], v[k + 3]);
                    mean /= (float)(v.size() / 4);
                }
            }
            tint *= display.maskMode() == 2 ? mean : glm::vec4(1, 1, 1, mean.a);
            if (tint.a <= 0.003f)
                return;
        }
    }
    glm::mat4 m = parent;
    if (n.transformation >= 0 && n.transformation < (int)gui.transformations.size()) {
        const plx::Object &t = gui.transformations[(size_t)n.transformation];
        glm::vec2 moved;
        const glm::vec2 *translation = nullptr;
        if (auto it = frame.shifts.find(n.name); it != frame.shifts.end())
            moved = plasma::translation(t) + it->second, translation = &moved;
        if (auto it = fills.find(n.name); it != fills.end()) {
            glm::mat4 d(1);
            d[0][0] = it->second;
            m = m * plasma::transformation(t, translation, &d);
        } else
            m = m * plasma::transformation(t, translation);
    }
    if (n.display >= 0 && n.display < (int)gui.displays.size()) {
        if (!plasma::displayVisible(gui.displays[(size_t)n.display]))
            return;
        if (const auto colour = frame.displayFills.find(n.name); colour != frame.displayFills.end())
            tint *= colour->second;
        else
            tint *= plasma::displayColor(gui.displays[(size_t)n.display]);
    }
    if (auto it = fills.find(n.name); it != fills.end() && it->second <= 0.0f)
        return; // (an empty fill, or a hidden charge bar)
    plasma::TextStyle style;
    if (clips) {
    } else if (n.shape >= 0 && n.shape < (int)gui.shapes.size() && plasma::textStyle(gui.shapes[(size_t)n.shape], gui.loaderFlags, style)) {
        if (text)
            style.string = *text;
        if (color)
            style.color = *color;
        if (hue)
            style.color = inHue(style.color, *hue), style.strokeColor = inHue(style.strokeColor, *hue);
        style.color.a *= tint.a, style.strokeColor.a *= tint.a; // (a clipping parent's fade)
        plasmaText(out, style, m);
    } else if (n.shape >= 0 && n.shape < (int)gui.shapes.size()) {
        auto found = shapes.find(n.shape);
        if (found == shapes.end()) {
            plasma::ShapeDrawing d;
            plasma::buildSmoothMesh(gui.shapes[(size_t)n.shape], d);
            found = shapes.emplace(n.shape, std::move(d)).first;
        }
        const plasma::ShapeDrawing &d = found->second;
        const float u = white.x + 2.f, v = white.y + 2.f;
        for (const plasma::Triangles *t : {&d.fill, &d.stroke}) { // the fill, then the stroke over it
            if ((t == &d.fill && !d.filled) || (t == &d.stroke && !d.stroked))
                continue;
            for (uint32_t i : t->indices) {
                const plasma::Vertex &p = t->vertices[i];
                const glm::vec4 s = m * glm::vec4(p.position, 0, 1);
                glm::vec4 c = glm::clamp(p.color * tint, glm::vec4(0), glm::vec4(1));
                if (hue)
                    c = inHue(c, *hue);
                c *= 255.0f;
                out.push_back({s.x / s.w + plasma::DRAW_OFFSET, s.y / s.w + plasma::DRAW_OFFSET, u, v, (uint8_t)c.r, (uint8_t)c.g,
                               (uint8_t)c.b, (uint8_t)c.a});
            }
        }
    }
    // The copied sprite child's binding supplies its physical frame. Original items
    // use the native WORLD helper; legacy/raw bindings retain their compatibility image.
    if (const auto sprite = frame.sprites.find(n.name);
        sprite != frame.sprites.end() && n.widget >= 0 && n.widget < (int)gui.widgets.size()) {
        const auto widget = plasma::reboundWidget(plasma::widgetOf(gui.widgets[(size_t)n.widget]));
        const auto content = glm::translate(m * widget.inverseBind, glm::vec3(widget.framePos, 0));
        const glm::vec2 origin(content * glm::vec4(0, 0, 0, 1));
        const glm::vec2 size(content * glm::vec4(widget.effectiveSize(), 0, 0));
        if (const auto original = frame.items.find(n.name); original != frame.items.end()) {
            if (original->second[0] != 0)
                itemIcon(sprite->second, original->second, guiitem::spritePlacement(origin, size, guiClockMs), out.size());
        } else if (const auto icon = icons.find(sprite->second); icon != icons.end()) {
            const glm::vec2 centre = origin + size * 0.5f;
            const float side = std::min(std::abs(size.x), std::abs(size.y));
            if (side > 0)
                image(out, icon->second.atlas, {icon->second.w, icon->second.h}, std::round(centre.x - side / 2),
                      std::round(centre.y - side / 2), side / (float)icon->second.w);
        }
    }
    for (int c : n.children)
        if (c >= 0 && c < (int)gui.nodes.size())
            plasmaNode(out, c, m, tint, frame, text, color, hue);
}

bool Hud::plasmaPlaced(std::vector<OverlayVertex> &out, const char *name, glm::vec2 position, glm::vec2 offset,
                       const PlasmaFrame &frame, glm::vec2 *centre, float deform) const {
    const int index = gui.find(name);
    if (index < 0)
        return false;
    const plx::Node &n = gui.nodes[(size_t)index];
    // placing a scene node under the interface's root (scaled by guiScale): the translation is
    // the point in the root's units, plus the offset, less the pivot.
    glm::mat4 root(1);
    root[0][0] = root[1][1] = guiScale;
    glm::vec2 translation = position / guiScale + offset;
    if (n.transformation >= 0 && n.transformation < (int)gui.transformations.size())
        translation -= plasma::pivot(gui.transformations[(size_t)n.transformation]);
    glm::mat4 m = root;
    const glm::mat4 deformation = glm::scale(glm::mat4(1), glm::vec3(deform, deform, 1.0f));
    if (n.transformation >= 0 && n.transformation < (int)gui.transformations.size())
        m = m * plasma::transformation(gui.transformations[(size_t)n.transformation], &translation, deform != 1.0f ? &deformation : nullptr);
    const size_t first = out.size();
    plx::Node top = n;
    top.transformation = -1; // (applied above)
    const_cast<plx::Document &>(gui).nodes.push_back(top);
    const int temporary = (int)gui.nodes.size() - 1;
    plasmaNode(out, temporary, m, glm::vec4(1), frame, nullptr);
    const_cast<plx::Document &>(gui).nodes.pop_back();
    if (centre && out.size() > first) {
        glm::vec2 lo(out[first].x, out[first].y), hi = lo;
        for (size_t i = first; i < out.size(); i++)
            lo = glm::min(lo, glm::vec2(out[i].x, out[i].y)), hi = glm::max(hi, glm::vec2(out[i].x, out[i].y));
        *centre = (lo + hi) * 0.5f;
    }
    return true;
}

void Hud::rect(std::vector<OverlayVertex> &out, float x, float y, float w, float h, glm::u8vec4 c) const {
    float u = white.x + 2.f, v = white.y + 2.f;
    OverlayVertex a{x, y, u, v, c.r, c.g, c.b, c.a}, b{x + w, y, u, v, c.r, c.g, c.b, c.a},
        d{x, y + h, u, v, c.r, c.g, c.b, c.a}, e{x + w, y + h, u, v, c.r, c.g, c.b, c.a};
    out.insert(out.end(), {a, b, d, b, e, d});
}

void Hud::image(std::vector<OverlayVertex> &out, glm::ivec2 at, glm::ivec2 size, float x, float y, float s) const {
    float w = size.x * s, h = size.y * s;
    float u0 = (float)at.x, v0 = (float)at.y, u1 = u0 + size.x, v1 = v0 + size.y;
    OverlayVertex a{x, y, u0, v0, 255, 255, 255, 255}, b{x + w, y, u1, v0, 255, 255, 255, 255},
        d{x, y + h, u0, v1, 255, 255, 255, 255}, e{x + w, y + h, u1, v1, 255, 255, 255, 255};
    out.insert(out.end(), {a, b, d, b, e, d});
}

float Hud::textWidth(const Font &font, const std::string &s) const {
    float w = 0;
    for (char c : s)
        if (auto it = font.glyphs.find((unsigned char)c); it != font.glyphs.end())
            w += it->second.advance;
    return w;
}

void Hud::text(std::vector<OverlayVertex> &out, const Font &font, const std::string &s, float x, float y,
               glm::u8vec4 fill, glm::u8vec4 outline, float t) const {
    if (font.smoothStroke) {
        for (int pass = 0; pass < 2; pass++) {
            float pen = x;
            for (char ch : s) {
                auto it = font.glyphs.find((unsigned char)ch);
                if (it == font.glyphs.end()) continue;
                const Glyph &g = it->second;
                int stroke = t > 4 * scale ? 1 : 0, pad = pass ? 0 : g.strokeRadius[stroke];
                glm::ivec2 at = pass ? g.atlas : g.strokeAtlas[stroke], size = g.size + glm::ivec2(pad * 2);
                if (g.size.x && g.size.y) {
                    float gx = std::round(pen + g.offset.x - pad), gy = std::round(y + font.ascent + g.offset.y - pad);
                    auto c = pass ? fill : outline;
                    OverlayVertex a{gx, gy, (float)at.x, (float)at.y, c.r, c.g, c.b, c.a},
                        b{gx + size.x, gy, (float)(at.x + size.x), (float)at.y, c.r, c.g, c.b, c.a},
                        d{gx, gy + size.y, (float)at.x, (float)(at.y + size.y), c.r, c.g, c.b, c.a},
                        e{gx + size.x, gy + size.y, (float)(at.x + size.x), (float)(at.y + size.y), c.r, c.g, c.b, c.a};
                    out.insert(out.end(), {a, b, d, b, e, d});
                }
                pen += g.advance;
            }
        }
        return;
    }
    // Outline: the glyphs stamped around in the outline colour, then the fill.
    for (int pass = 0; pass < 2; pass++) {
        static const int offsets[9][2] = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}, {0, 0}};
        for (int o = pass ? 8 : 0; o < (pass ? 9 : 8); o++) {
            glm::u8vec4 c = pass ? fill : outline;
            float pen = x;
            for (char ch : s) {
                auto it = font.glyphs.find((unsigned char)ch);
                if (it == font.glyphs.end())
                    continue;
                const Glyph &g = it->second;
                if (g.size.x > 0) {
                    float gx = std::round(pen + g.offset.x + offsets[o][0] * t),
                          gy = std::round(y + font.ascent + g.offset.y + offsets[o][1] * t);
                    float u0 = (float)g.atlas.x, v0 = (float)g.atlas.y, u1 = u0 + g.size.x, v1 = v0 + g.size.y;
                    OverlayVertex a{gx, gy, u0, v0, c.r, c.g, c.b, c.a},
                        b{gx + g.size.x, gy, u1, v0, c.r, c.g, c.b, c.a},
                        d{gx, gy + g.size.y, u0, v1, c.r, c.g, c.b, c.a},
                        e{gx + g.size.x, gy + g.size.y, u1, v1, c.r, c.g, c.b, c.a};
                    out.insert(out.end(), {a, b, d, b, e, d});
                }
                pen += g.advance;
            }
        }
    }
}

void Hud::bar(std::vector<OverlayVertex> &out, const Bar &b, float fraction, const std::string &value) const {
    auto X = [&](auto x) { return offsetX + (float)x * scale; }; // whole pixels of the 1920x1080 art
    auto Y = [&](auto y) { return (float)y * scale; };
    fraction = std::clamp(fraction, 0.f, 1.f);
    int rows = b.interior.w;
    for (int r = 0; r < rows; r++) {
        float y = Y(b.interior.y + r), h = Y(b.interior.y + r + 1) - y;
        float a = r < (int)b.emptyAlpha.size() ? b.emptyAlpha[r] : .3f;
        rect(out, X(b.interior.x), y, b.interior.z * scale, h, {0, 0, 0, (uint8_t)(a * 255)});
        if (fraction > 0 && r < (int)b.fill.size()) {
            glm::u8vec3 c = b.fill[r];
            rect(out, X(b.interior.x), y, b.interior.z * scale * fraction, h, {c.r, c.g, c.b, 255});
        }
    }
    image(out, b.atlas, {b.imageW, b.imageH}, X(b.frame.x), Y(b.frame.y), scale);
    if (!value.empty()) {
        float w = textWidth(valueFont, value);
        // Digits centred in the interior (their tops sit ascent - height below y).
        float x = X(b.interior.x + b.interior.z / 2.f) - w / 2,
              y = Y(b.interior.y + b.interior.w / 2.f) + VALUE_DIGITS * scale / 2 - valueFont.ascent;
        text(out, valueFont, value, std::round(x), std::round(y), {b.text, 255},
             {b.outline, 255}, std::max(1.f, 2 * scale));
    }
}

bool Hud::loadQuestIcons(const std::filesystem::path &file) {
    int w = 0, h = 0, n = 0;
    unsigned char *px = stbi_load(file.string().c_str(), &w, &h, &n, 4);
    if (!px)
        return false;
    // The exact crops (with their white outlines); the noisy green is keyed out by hue,
    // keeping every pixel of the orange, grey and white art (max(r, b) * 1.3 > g).
    struct Crop {
        int x, y, w, h;
        const char *key;
    };
    static const Crop crops[] = {{16, 16, 48, 52, "quest-event"}, {96, 16, 48, 52, "quest-destination"}, {188, 16, 24, 48, "quest-offer"}};
    for (const Crop &c : crops) {
        if (c.x + c.w > w || c.y + c.h > h)
            continue;
        std::vector<uint8_t> rgba((size_t)(c.w * c.h * 4));
        for (int y = 0; y < c.h; y++)
            for (int x = 0; x < c.w; x++) {
                const unsigned char *s = px + ((size_t)(c.y + y) * w + (size_t)(c.x + x)) * 4;
                uint8_t *d = &rgba[((size_t)y * c.w + (size_t)x) * 4];
                d[0] = s[0], d[1] = s[1], d[2] = s[2];
                d[3] = (float)std::max(s[0], s[2]) * 1.3f - (float)s[1] > 0.0f ? 255 : 0;
            }
        addIcon(c.key, std::move(rgba), c.w, c.h);
    }
    stbi_image_free(px);
    return true;
}

// The map overlay's markers (docs/specs/map-markers.md 1.8), far ones first. Each is an icon
// 15 interface units above its point (bobbing unless it is a shrine or carries a mark), a mark
// over the icon ("solved", always bobbing), its label under the icon, and a note over the label.
// Texts are drawn twice: white with a darker outline, then their colour without one.
void Hud::drawMapMarkers(const State &state, std::vector<OverlayVertex> &out) const {
    using namespace mapmarks;
    auto collected = state.collectMapMarkers ? state.collectMapMarkers() : std::vector<Marker>{};
    if (state.collectMapMarkers)
        collected.insert(collected.end(), state.mapMarkers.begin(), state.mapMarkers.end());
    const auto &markers = state.collectMapMarkers ? collected : state.mapMarkers;
    Hover hovered;
    if (state.mapInteractive)
        hovered =
            hover(markers, state.pointerAt, guiScale, [&](const Marker &marker) { return mapLabelWidth(marker); }, state.mapShrineMode);
    mapOutputs.admittedDraw(&hovered);
    if (state.mapOverlayDraw)
        state.mapOverlayDraw(hovered);
    const Cell target = state.mapOverlayDraw ? (hovered.shrine ? Cell{hovered.shrine->x, hovered.shrine->y}
                                                               : Cell{PreviewOutputs::NONE, PreviewOutputs::NONE})
                                             : state.mapShrineTarget;
    const float bobbing = bob(state.mapClockMs);
    auto outlinedText = [&](const std::string &text, glm::vec2 at, float scale, glm::vec4 fill, glm::vec4 outline) {
        plasma::TextStyle s = labelStyle((int)(scale * 12.0f * guiScale), (int)(scale * 3.0f * guiScale), plasma::TEXT_CENTER, true);
        s.string.clear();
        for (char c : text) // (the labels' stars are U+00B1, the font's star)
            s.string.push_back((wchar_t)(unsigned char)c);
        glm::mat4 m(1);
        m[3] = glm::vec4(at.x, at.y, 0, 1);
        s.color = glm::vec4(1);
        s.strokeColor = outline;
        plasmaText(out, s, m);
        s.strokeRadius = 0;
        s.color = fill;
        plasmaText(out, s, m);
    };
    for (const State::MapMarker &k : markers) {
        if (!k.questSprite.empty()) { // (OverworldEvents' quest sprites: anchored at their bottom centre)
            if (auto icon = icons.find(k.questSprite); icon != icons.end()) {
                const float scale = std::clamp(k.scale, 0.25f, 2.0f);
                const float w = (float)icon->second.w * scale, h = (float)icon->second.h * scale;
                image(out, icon->second.atlas, {icon->second.w, icon->second.h}, std::round(k.at.x - w * 0.5f), std::round(k.at.y - h), scale);
            }
            continue;
        }
        float labelY = k.at.y;
        if (!k.icon.empty() && guiLoaded) {
            const bool still = k.shrineTarget || !k.overlay.empty();
            const float rise = ICON_RISE - (still ? 0.0f : bobbing);
            const bool enlarged = state.mapInteractive ? hoveredTarget(k, hovered) : k.enlarged;
            plasmaPlaced(out, k.icon.c_str(), k.at, glm::vec2(0, -rise), PlasmaFrame{}, nullptr, enlarged ? HOVERED_ICON_SCALE : k.scale);
            labelY += (float)(int)(LABEL_DROP * guiScale);
        }
        if (!k.overlay.empty() && guiLoaded)
            plasmaPlaced(out, k.overlay.c_str(), k.at, glm::vec2(0, -(ICON_RISE - bobbing)), PlasmaFrame{}, nullptr, k.scale);
        const auto text = markerText(k, state.mapShrineMode, target, state.mapWorldElapsedMs);
        if (!k.label.empty())
            outlinedText(k.label, glm::vec2(k.at.x, labelY), text.scale, text.colour, glm::vec4(glm::vec3(text.colour) * 0.3f, 1.0f));
        if (!k.note.empty())
            outlinedText(k.note, glm::vec2(k.at.x, labelY - NOTE_RISE * guiScale), k.scale, glm::vec4(1.0f, 0.8f, 0.1f, 1.0f),
                         glm::vec4(0.6f, 0.25f, 0.0f, 1.0f));
    }
}

float Hud::mapLabelWidth(const State::MapMarker &marker) const {
    if (!plasmaFonts[0].loaded())
        return 0.0f;
    plasma::TextStyle style = labelStyle((int)(marker.scale * 12.0f * guiScale), (int)(marker.scale * 3.0f * guiScale), 0, true);
    for (char c : marker.label)
        style.string.push_back((wchar_t)(unsigned char)c);
    const plasma::Raster r = plasma::rasterFor(glm::mat4(1), style);
    const plasma::TextBox box = plasma::measure(plasmaFonts[0], style.string, r, 0, -1, true);
    return box.maximum.x - box.minimum.x;
}

void Hud::draw(const State &state, std::vector<OverlayVertex> &out,
               const std::function<void(std::vector<OverlayVertex> &)> &overlays) const {
    out.clear();
    itemRequests.clear();
    guiClockMs = state.guiClockMs;
    guiStepMs = state.guiStepMs;
    if (!ready)
        return;
    if (panels.customization && !state.customization)
        panels.customization->visible = false;
    const Inventory *information = state.information ? state.information : state.inventory;
    if (information && (state.mapInformation || (state.mapPreview.empty() && information->infoItem)))
        positionPreview(*information); // Selection observes the preceding draw's stored frame.
    ringItems(state.ring);
    if (!state.interfaceVisible)
        return;
    if (state.feats)
        drawFeatText(*state.feats, out);
    // Over-head plates and damage numbers first, so the bars stay on top.
    for (auto &p : state.plates) {
        float w = PLATE_W * scale, h = PLATE_H * scale, x = std::round(p.at.x - w / 2), y = std::round(p.at.y - h);
        float px = std::max(1.f, std::round(2 * scale));
        rect(out, x, y, w, h, BLACK);
        float ix = x + px, iy = y + px, iw = w - 2 * px, ih = h - 2 * px;
        float health = std::clamp(p.health, 0.f, 1.f), trail = std::clamp(std::max(p.trail, p.health), 0.f, 1.f);
        rect(out, ix, iy, iw, ih, PLATE_EMPTY);
        rect(out, ix, iy, iw * trail, ih, PLATE_TRAIL);
        rect(out, ix, iy, iw * health, ih, PLATE_FILL);
        rect(out, ix, iy, iw, px, PLATE_TOP);
        rect(out, ix, iy + ih - px, iw, px, PLATE_BOTTOM);
        std::string label = p.level > 0 ? "LV " + std::to_string(p.level) + " " : "";
        for (char c : p.name)
            label += (char)std::toupper((unsigned char)c);
        float tw = textWidth(plateFont, label);
        text(out, plateFont, label, std::round(p.at.x - tw / 2), y - plateFont.ascent - 4 * scale,
             {255, 255, 255, 255}, BLACK, std::max(1.f, 2 * scale));
    }
    for (auto &n : state.numbers) {
        std::string s = std::to_string(n.value);
        float tw = textWidth(damageFont, s);
        uint8_t a = (uint8_t)(255 * std::clamp(n.fade, 0.f, 1.f));
        glm::u8vec4 fill = n.taken ? DAMAGE_FILL : glm::u8vec4(255, 255, 255, 255), outline = DAMAGE_OUTLINE;
        fill.a = outline.a = a;
        text(out, damageFont, s, std::round(n.at.x - tw / 2), std::round(n.at.y), fill, outline,
             std::max(1.f, 2 * scale));
    }
    // Whole points, rounded down as the game shows them (145.6 max -> "145/145").
    auto value = [](float v, float m) {
        int max = (int)m, now = std::min((int)std::max(v, 0.f), max);
        return std::to_string(now) + "/" + std::to_string(max);
    };
    if (state.stamina < .999f) { // The game's stamina bar: small and blue, under the character.
        float w = 105 * scale, h = 9 * scale, px = std::max(1.f, std::round(2 * scale));
        float x = std::round(state.staminaAt.x - w / 2), y = std::round(state.staminaAt.y + 10 * scale);
        rect(out, x, y, w, h, BLACK);
        rect(out, x + px, y + px, w - 2 * px, h - 2 * px, {20, 34, 48, 230});
        rect(out, x + px, y + px, (w - 2 * px) * std::clamp(state.stamina, 0.f, 1.f), h - 2 * px, {96, 190, 255, 255});
    }
    // Speech bubbles: the NPC's line in a white box over its head, wrapped.
    for (auto &b : state.bubbles) {
        std::vector<std::string> lines(1);
        std::istringstream words(b.text);
        std::string word;
        const float wrap = 420 * scale;
        while (words >> word) {
            std::string tryLine = lines.back().empty() ? word : lines.back() + " " + word;
            if (!lines.back().empty() && textWidth(labelFont, tryLine) > wrap)
                lines.push_back(word);
            else
                lines.back() = tryLine;
        }
        float w = 0;
        for (auto &l : lines)
            w = std::max(w, textWidth(labelFont, l));
        float pad = 12 * scale, lineH = 22 * scale, h = lines.size() * lineH + pad * 2 - 6 * scale;
        float x = std::round(b.at.x - w / 2 - pad), y = std::round(b.at.y - h - 12 * scale);
        uint8_t a = (uint8_t)(255 * std::clamp(b.fade, 0.f, 1.f));
        float px = std::max(1.f, std::round(2 * scale));
        rect(out, x - px, y - px, w + pad * 2 + 2 * px, h + 2 * px, {0, 0, 0, a});
        rect(out, x, y, w + pad * 2, h, {250, 250, 245, a});
        rect(out, std::round(b.at.x - 6 * scale), y + h, 12 * scale, 8 * scale, {250, 250, 245, a}); // the tail
        for (size_t i = 0; i < lines.size(); i++)
            text(out, labelFont, lines[i], std::round(b.at.x - textWidth(labelFont, lines[i]) / 2),
                 std::round(y + pad + i * lineH + 11 * scale - labelFont.ascent), {30, 30, 36, a}, {250, 250, 245, a}, 0);
    }
    // The quest tracker, at the right.
    if (!state.quest.empty()) {
        float y = 300 * scale, right = offsetX + 1890 * scale;
        for (size_t i = 0; i < state.quest.size(); i++) {
            const std::string &line = state.quest[i];
            float tw = textWidth(labelFont, line);
            text(out, labelFont, line, std::round(right - tw), std::round(y + 11 * scale - labelFont.ascent),
                 i == 0 ? glm::u8vec4(255, 214, 96, 255) : glm::u8vec4(255, 255, 255, 255), BLACK, std::max(1.f, 2 * scale));
            y += 26 * scale;
        }
    }
    for (auto &m : state.messages) {
        const Font &f = m.big ? damageFont : plateFont;
        float tw = textWidth(f, m.text);
        glm::u8vec4 fill = m.color, outline = BLACK;
        fill.a = outline.a = (uint8_t)(255 * std::clamp(m.fade, 0.f, 1.f));
        text(out, f, m.text, std::round(m.at.x - tw / 2), std::round(m.at.y), fill, outline, std::max(1.f, 2 * scale));
    }
    if (state.showXp && guiLoaded) { // PyroProgression's XP: the mana bar's art in its purple, centred over the two
        PlasmaFrame xpFrame;
        xpFrame.fills = {{"bar", std::clamp((float)state.xp / (float)std::max(1, state.xpMax), 0.0f, 1.0f)}, {"chargebar", 0.0f}};
        xpFrame.texts = {{"text", std::to_wstring(state.xp) + L"/" + std::to_wstring(state.xpMax)}, {"info", L"XP"}};
        xpFrame.hues = {{"bar", 266.0f}, {"text", 266.0f}};
        plasmaPlaced(out, "mpbar", {screen.x / 2, screen.y}, {-86.5f, -82}, xpFrame);
    } else if (auto it = bars.find("xp"); it != bars.end() && !guiLoaded)
        bar(out, it->second, state.showXp ? (float)state.xp / std::max(1, state.xpMax) : 0,
            state.showXp ? std::to_string(state.xp) + "/" + std::to_string(state.xpMax) : "");
    if (state.gold >= 0 && guiLoaded) { // The game's money (hud-money.md): the "gold" text; its turning coin is a 3D model (src/HudCoin.*)
        plasmaLabel(out, std::to_wstring(state.gold), {0, 0}, {85, 60}, 24, 5);
    } else if (state.gold >= 0) { // The game's money counter, top left.
        if (auto it = icons.find("gold-coin"); it != icons.end())
            image(out, it->second.atlas, {it->second.w, it->second.h}, std::round(offsetX + 28 * scale),
                  std::round(26 * scale), 52 * scale / it->second.w);
        text(out, damageFont, std::to_string(state.gold), std::round(offsetX + 112 * scale),
             std::round(66 * scale - damageFont.ascent), {255, 255, 255, 255}, BLACK, std::max(1.f, 2 * scale));
    }
    // The health and mana bars: the game's own shapes and text (the interface's layout for the window size:
    // the bottom middle, offset -181/8 and -50; Interface's update: the "bar" child scaled by
    // the fraction, the "text" shapes given "hp/max" and "mana/100").
    const float hpFill = std::min(1.0f, std::max(0.0f, state.hp) / std::max(state.maxHp, 1e-6f));
    const float mpFill = std::clamp(state.mp / std::max(state.maxMp, 1e-6f), 0.0f, 1.0f);
    auto number = [](int v) { // Interface's the number format
        const std::string s = v >= 1000000 ? std::to_string(v / 1000000) + "." + std::to_string(v % 1000000 / 100000) + "M"
                              : v >= 10000 ? std::to_string(v / 1000) + "." + std::to_string(v % 1000 / 100) + "K"
                                           : std::to_string(v);
        return std::wstring(s.begin(), s.end());
    };
    PlasmaFrame hpFrame, mpFrame;
    hpFrame.fills = {{"bar", hpFill}};
    hpFrame.texts = {{"text", number((int)state.hp) + L"/" + number((int)state.maxHp)}};
    mpFrame.fills = {{"bar", mpFill}, {"chargebar", 0.0f}};
    mpFrame.texts = {{"text", std::to_wstring((int)state.mp) + L"/100"}};
    // The rest of the in-game interface the game draws from gui.plx (the interface's layout for the window size's
    // places; Interface's and Game's updates fill them): the cast and stamina bars, the quick
    // buttons, the crosshairs.
    const glm::vec2 bottomMiddle(std::floor(screen.x / 2), screen.y), middle(std::floor(screen.x / 2), std::floor(screen.y / 2));
    auto wide = [](const std::string &s) { // (Latin-1: the rarity mark is 0xb1)
        std::wstring w;
        for (unsigned char c : s)
            w.push_back((wchar_t)c);
        return w;
    };
    if (guiLoaded) {
        if (state.cast) {
            PlasmaFrame f;
            f.fills = {{"bar", state.castFill}};
            f.texts = {{"name", wide(state.castName)}};
            plasmaPlaced(out, "castbar", bottomMiddle, {0, -200}, f);
        }
        if (state.stamina < 1.0f) {
            PlasmaFrame f;
            f.fills = {{"bar", std::max(0.0f, state.stamina)}};
            plasmaPlaced(out, "staminabar", bottomMiddle, {0, -180}, f);
        }
        auto quick = [&](const State::Quick &q, glm::vec2 at, glm::vec2 offset) {
            PlasmaFrame f;
            f.texts = {{"name", wide(q.key)}, {"info", wide(q.info)}, {"count", wide(q.count)}};
            f.colors = {{"name", glm::vec4(1)}, {"info", q.infoColor}};
            if (!q.icon.empty())
                f.sprites = {{"sprite", q.icon}};
            if (q.original)
                f.items = {{"sprite", *q.original}};
            plasmaPlaced(out, "quickbutton", at, offset, f);
        };
        if (state.ring.shown) {
            PlasmaFrame ring;
            ring.texts = {{"name", wide(state.ring.name)}, {"count", wide(state.ring.count)}};
            ring.colors = {{"name", state.ring.colour}};
            ring.displayFills = {{"left", state.ring.left ? glm::vec4(1, 0, .2f, 1) : glm::vec4(1)},
                                 {"right", state.ring.right ? glm::vec4(1, 0, .2f, 1) : glm::vec4(1)}};
            if (state.ring.items.empty())
                ring.hidden.insert("brackets");
            plasmaPlaced(out, "selector", middle, {0, -200}, ring);
        }
        quick(state.quickItem, {0, screen.y}, {30, -80});
        quick(state.quickUse, bottomMiddle, {-23, -150});
        quick(state.quickSkill, {0, screen.y}, {90, -110});
        PlasmaFrame cross;
        cross.texts = {{"crosshair", L""}}; // (the character control clears its text)
        if (state.crosshair)
            plasmaPlaced(out, "crosshair", middle, {0, 0}, cross);
        if (state.zoomCrosshair)
            plasmaPlaced(out, "zoomcrosshair", middle, {0, 0}, cross);
    }
    if (guiLoaded) { // the info texts and the location banner
        plasmaLabel(out, wide(state.infoSmall), {screen.x, 0}, {-120, 20}, 12, 3, plasma::TEXT_RIGHT);
        plasmaLabel(out, wide(state.infoLarge), {screen.x, 0}, {-120, 60}, 28, 6, plasma::TEXT_RIGHT);
        plasmaLabel(out, wide(state.infoMedium), {screen.x, 0}, {-120, 100}, 21, 5, plasma::TEXT_RIGHT);
        if (state.locationMs >= 0) {
            PlasmaFrame f;
            f.texts = {{"name", wide(state.locationName)}, {"detail", L""}};
            f.sequence = "location";
            f.sequenceMs = state.locationMs;
            plasmaPlaced(out, "location", {std::floor(screen.x / 2), std::floor(screen.y / 3)}, {0, 0}, f);
        }
    }
    if (guiLoaded) // the creatures' life bars (under the rest: the game's "lifebars" node comes first)
        for (const State::LifeBar &b : state.lifeBars) {
            static const char *const kinds[4] = {"enemylifebar", "friendlifebar", "staticlifebar", "neutrallifebar"};
            const std::string node = std::string(kinds[std::clamp(b.kind, 0, 3)]) + (b.big ? "" : ":small");
            PlasmaFrame f;
            f.fills = {{"bar", b.fraction}, {"bar2", 1.0f - b.fraction}};
            f.texts = {{"rarename", wide(b.name)}, {"faction", wide(b.faction)}, {"info", wide(b.info)}, {"text", wide(b.text)}};
            f.colors = {{"info", b.infoColor}};
            if (!b.big)
                f.hidden = {"rarename", "info", "faction"};
            if (b.onlyInfo)
                f.hidden = {"bar", "bar2", "frame", "rarename", "text", "faction"};
            if (!b.faction.empty())
                f.shifts = {{"info", {0, -12}}, {"rarename", {0, -12}}, {"faction", {0, -12}}};
            plasmaPlaced(out, node.c_str(), {b.at.x, std::max(b.at.y, guiScale * 50.0f)}, {0, 0}, f);
        }
    if (!guiLoaded || !plasmaPlaced(out, "hpbar", {screen.x / 2, screen.y}, {-181, -50}, hpFrame) ||
        !plasmaPlaced(out, "mpbar", {screen.x / 2, screen.y}, {8, -50}, mpFrame)) {
        if (auto it = bars.find("hp"); it != bars.end())
            bar(out, it->second, state.hp / state.maxHp, value(state.hp, state.maxHp));
        if (auto it = bars.find("mp"); it != bars.end())
            bar(out, it->second, state.mp / state.maxMp, value(state.mp, state.maxMp));
    }
    if (state.inventory)
        drawInventory(*state.inventory, out);
    if (state.identification)
        drawIdentification(*state.identification, out);
    if (state.customization)
        drawCustomization(*state.customization, out);
    if (state.windows)
        drawScreen(*state.windows, out);
    drawMapMarkers(state, out); // The game-UI subtree precedes the separate MapOverlay root.
    drawMapHistory(out);
    if (state.mapInformation)
        drawNativeMapPreview(state, out);
    else if (state.mapPreview.empty()) {
        if (state.informationDraw)
            state.informationDraw();
        if (information)
            drawPreview(*information, out);
    }
    if (overlays)
        overlays(out);
    if (!state.mapInformation)
        drawMapPreview(state, out);
    drawPointer(state, out);
}

void Hud::drawPointer(const State &state, std::vector<OverlayVertex> &out) const {
    if (!state.pointer || !cursorNode)
        return;
    cursorScene->setPosition(*cursorNode, state.pointerAt, glm::vec2(0));
    cursorNode->setAllText(std::wstring(state.pointerText.begin(), state.pointerText.end()));
    sceneNode(out, *cursorNode, glm::vec4(1), cursorScene.get());
}

// --- Inventory screen ------------------------------------------------------------
// Positions are the game's at 1920x1080, as measured in the captured frame.
namespace {
constexpr float LIST_X = 712, LIST_Y = 117, LIST_W = 494, LIST_H = 355; // panel inside
constexpr float ROW_Y = 160, ROW_H = 56, ICON = 52;
constexpr float SLOT_LEFT = 705, SLOT_RIGHT = 1143, SLOT_TOP = 485, SLOT_PITCH = 70, SLOT_SIZE = 72;
const char *SLOT_LABELS[10][2] = {{"LEFT", "WEAPON"}, {"RIGHT", "WEAPON"}, {"LEFT RING", ""}, {"RIGHT RING", ""},
                                  {"NECK", ""},       {"SHOULDER", ""},     {"CHEST", ""},     {"HANDS", ""},
                                  {"FEET", ""},       {"PET", ""}};
} // namespace

void Hud::addIcon(const std::string &key, std::vector<uint8_t> rgba, int size) {
    addIcon(key, std::move(rgba), size, size);
}

void Hud::addIcon(const std::string &key, std::vector<uint8_t> rgba, int w, int h) {
    Picture &p = icons[key];
    p.rgba = std::move(rgba);
    p.w = w;
    p.h = h;
    atlasDirty = true;
}

glm::vec2 Hud::slotPosition(int slot) const {
    return {slot % 2 ? SLOT_RIGHT : SLOT_LEFT, SLOT_TOP + (slot / 2) * SLOT_PITCH};
}

int Hud::inventoryItemAt(glm::vec2 m, const Inventory &inv) const {
    return hasPanels() ? panelItemAt(m, inv) : inventoryItemAt(m, inv.items.size());
}
int Hud::inventorySlotAt(glm::vec2 m, const Inventory &inv) const {
    return hasPanels() ? panelSlotAt(m, inv) : inventorySlotAt(m);
}

int Hud::inventoryItemAt(glm::vec2 m, size_t items) const {
    float x = (m.x - offsetX) / scale, y = m.y / scale;
    if (x < LIST_X || x > LIST_X + LIST_W - 40 || y < ROW_Y - 8)
        return -1;
    int row = (int)((y - (ROW_Y - 8)) / ROW_H);
    return row < (int)items && y < LIST_Y + LIST_H ? row : -1;
}

int Hud::inventorySlotAt(glm::vec2 m) const {
    float x = (m.x - offsetX) / scale, y = m.y / scale;
    for (int s = 0; s < 10; s++) {
        glm::vec2 p = slotPosition(s);
        if (x >= p.x && x < p.x + SLOT_SIZE && y >= p.y && y < p.y + SLOT_SIZE)
            return s;
    }
    return -1;
}

void Hud::drawInventory(const Inventory &inv, std::vector<OverlayVertex> &out) const {
    if (hasPanels())
        drawPanels(inv, out);
    if (!inventoryReady)
        return;
    auto X = [&](float x) { return std::round(offsetX + x * scale); };
    auto Y = [&](float y) { return std::round(y * scale); };
    auto colour = [&](const char *name, glm::u8vec3 fallback) {
        auto it = colours.find(name);
        glm::u8vec3 c = it == colours.end() ? fallback : it->second;
        return glm::u8vec4(c, 255);
    };
    const glm::u8vec4 black{0, 0, 0, 255};
    float t = std::max(1.f, std::round(2 * scale));
    // Text whose digits/capitals are `height` tall with their tops at y.
    auto write = [&](const Font &f, float capHeight, const std::string &s, float x, float y, glm::u8vec4 c) {
        text(out, f, s, std::round(x), std::round(y + capHeight * scale - f.ascent), c, black, t);
    };
    auto icon = [&](const std::string &key, float x, float y, float size) {
        auto it = icons.find(key);
        if (it != icons.end())
            image(out, it->second.atlas, {it->second.w, it->second.h}, X(x), Y(y), size * scale / it->second.w);
    };

    if (hasPanels() && inv.sheet.valid)
        return; // (the character panel, the bag and the slots are the game's own)
    // Character panel (left).
    write(nameFont, 15, inv.name, X(24), Y(199), colour("name", {255, 204, 51}));
    write(nameFont, 15, "LEVEL " + std::to_string(inv.level), X(24), Y(229), colour("level", {255, 255, 255}));
    float row = 312;
    auto stat = [&](const std::string &label, const std::string &value) {
        write(labelFont, 11, label, X(24), Y(row), colour("stat", {133, 146, 198}));
        write(labelFont, 11, value, X(322), Y(row), colour("value", {153, 255, 153}));
        row += 20;
    };
    stat("CLASS", inv.className);
    stat("SPECIALIZATION", inv.specialization);
    row += 5;
    for (auto &[label, value] : inv.stats)
        stat(label, value);
    if (hasPanels())
        return; // (the bag and the slots are the game's own panels)

    // Tabs and the item list (equipment tab).
    image(out, tabs.atlas, {tabs.w, tabs.h}, X(712), Y(44), scale);
    rect(out, X(LIST_X), Y(LIST_Y), LIST_W * scale, LIST_H * scale, {0, 0, 0, (uint8_t)(panelDark * 255)});
    image(out, panel.atlas, {panel.w, panel.h}, X(704), Y(107), scale);
    write(labelFont, 11, "EQUIPMENT", X(728), Y(136), colour("header", {255, 204, 128}));
    for (size_t i = 0; i < inv.items.size(); i++) {
        float y = ROW_Y + i * ROW_H;
        if (y + ROW_H > LIST_Y + LIST_H)
            break;
        if ((int)i == inv.hoverItem)
            rect(out, X(LIST_X + 4), Y(y - 6), (LIST_W - 44) * scale, (ROW_H - 4) * scale, {255, 255, 255, 40});
        icon(inv.items[i].icon, 722, y - 4, ICON);
        std::string name = inv.items[i].name;
        if (inv.items[i].count > 1)
            name += " (" + std::to_string(inv.items[i].count) + ")";
        // Items the player's class cannot use are dimmed.
        glm::u8vec4 c = inv.items[i].allowed ? colour("item", {76, 115, 255}) : glm::u8vec4(110, 110, 120, 255);
        write(labelFont, 11, name, X(785), Y(y + 9), c);
    }

    // Equipment slots around the character.
    for (int s = 0; s < 10; s++) {
        glm::vec2 p = slotPosition(s);
        rect(out, X(p.x + 7), Y(p.y + 7), (SLOT_SIZE - 14) * scale, (SLOT_SIZE - 14) * scale,
             {0, 0, 0, (uint8_t)(slotDark * 255)});
        if (s == inv.hoverSlot)
            rect(out, X(p.x + 7), Y(p.y + 7), (SLOT_SIZE - 14) * scale, (SLOT_SIZE - 14) * scale, {255, 255, 255, 40});
        image(out, slotFrame.atlas, {slotFrame.w, slotFrame.h}, X(p.x), Y(p.y), scale);
        icon(inv.slots[s], p.x + 10, p.y + 10, SLOT_SIZE - 20);
        const char *l1 = SLOT_LABELS[s][0], *l2 = SLOT_LABELS[s][1];
        bool two = *l2 != 0;
        float ly = p.y + (two ? 22 : 31);
        for (const char *l : {l1, l2}) {
            if (!*l)
                continue;
            float w = textWidth(labelFont, l);
            float lx = s % 2 ? X(SLOT_RIGHT - 10) - w : X(SLOT_LEFT + 78);
            write(labelFont, 11, l, lx, Y(ly), colour("label", {255, 255, 255}));
            ly += 17;
        }
    }
}
