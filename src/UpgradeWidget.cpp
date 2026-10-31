// The weapon upgrade widget (docs/specs/loot-crafting.md §4): its palette, what a click does, and
// the weapon drawn and picked through its voxels. See UpgradeWidget.h.
#include "UpgradeWidget.h"

#include "GameCraft.h"
#include "NativeInteract.h"
#include "NativePose.h"
#include "ScreenItems.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace upgradewidget {
namespace {

// Item record fields (drops-and-items.md §0.1).
constexpr size_t TYPE = 0x00, SUBTYPE = 0x04, MATERIAL = 0x1c;
constexpr size_t CUBES = 0x1d, CUBE_SIZE = 4, CUBE_COUNT = 0x9d;
constexpr int MOST_CUBES = 32; // the record's room for entries

int32_t subtypeOf(const Item &item) {
    int32_t v;
    std::memcpy(&v, &item[SUBTYPE], 4);
    return v;
}

int cubeCount(const uint8_t *weapon) { return std::min<int>(weapon[CUBE_COUNT], MOST_CUBES); }
Voxel cubeAt(const uint8_t *weapon, int i) {
    const uint8_t *c = weapon + CUBES + CUBE_SIZE * (size_t)i;
    return {(int8_t)c[0], (int8_t)c[1], (int8_t)c[2]};
}
uint8_t cubeMaterial(const uint8_t *weapon, int i) { return weapon[CUBES + CUBE_SIZE * (size_t)i + 3]; }

// --- The weapon's voxels as the view sees them ------------------------------------------------

// The model's box grown by one voxel each way, as a grid whose cell (0, 0, 0) is the model's
// voxel (-1, -1, -1).
struct Grid {
    enum Kind : uint8_t { EMPTY, WEAPON, CUBE };
    int nx = 0, ny = 0, nz = 0;
    std::vector<uint8_t> kind;
    std::vector<glm::u8vec3> colour;
    size_t index(int x, int y, int z) const { return ((size_t)z * ny + y) * nx + x; }
    bool inside(int x, int y, int z) const { return x >= 0 && y >= 0 && z >= 0 && x < nx && y < ny && z < nz; }
};

Grid gridOf(const CubModel &model, const uint8_t *weapon) {
    Grid g;
    g.nx = model.sx + 2, g.ny = model.sy + 2, g.nz = model.sz + 2;
    g.kind.assign((size_t)g.nx * g.ny * g.nz, Grid::EMPTY);
    g.colour.assign(g.kind.size(), glm::u8vec3(0));
    for (int z = 0; z < model.sz; z++)
        for (int y = 0; y < model.sy; y++)
            for (int x = 0; x < model.sx; x++)
                if (model.solid(x, y, z)) {
                    const size_t i = g.index(x + 1, y + 1, z + 1);
                    const VoxelColor c = model.color(x, y, z);
                    g.kind[i] = Grid::WEAPON, g.colour[i] = {c.r, c.g, c.b};
                }
    if (weapon)
        for (int i = 0; i < cubeCount(weapon); i++) { // (drawn over a weapon voxel at the same place)
            const Voxel v = cubeAt(weapon, i);
            if (!g.inside(v.x + 1, v.y + 1, v.z + 1))
                continue;
            const size_t k = g.index(v.x + 1, v.y + 1, v.z + 1);
            const glm::vec4 c = glm::clamp(cubeColour(cubeMaterial(weapon, i)), glm::vec4(0), glm::vec4(1)) * 255.0f;
            g.kind[k] = Grid::CUBE, g.colour[k] = glm::u8vec3(c);
        }
    return g;
}

// The view's frame in the model's voxel space: the box's middle, the screen's right and up and
// the direction toward the viewer, and the picture's pixels per voxel. The scale fits the box
// as seen from every eighth of a turn, so turning the view does not change the weapon's size.
struct Frame {
    glm::vec3 centre{0}, right{1, 0, 0}, up{0, 0, 1}, toViewer{0, 1, 0};
    float scale = 1, half = 0;
};
glm::vec3 viewerDirection(float yawDegrees, float pitchDegrees) {
    const float yaw = glm::radians(yawDegrees), pitch = glm::radians(pitchDegrees);
    return {std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw), std::sin(pitch)};
}
glm::vec3 rightOf(float yawDegrees) {
    const float yaw = glm::radians(yawDegrees);
    return {-std::sin(yaw), std::cos(yaw), 0.0f};
}
constexpr float FILL = 0.94f; // the share of the picture the box may take

Frame frameOf(const CubModel &model, const View &view) {
    Frame f;
    const glm::vec3 low(-1.0f), high(model.sx + 1.0f, model.sy + 1.0f, model.sz + 1.0f);
    f.centre = (low + high) * 0.5f;
    f.half = (float)view.pixels * 0.5f;
    float extent = 0.5f;
    for (int step = 0; step < 8; step++) {
        const float yaw = view.yawDegrees + 45.0f * (float)step;
        const glm::vec3 d = viewerDirection(yaw, view.pitchDegrees), r = rightOf(yaw), u = glm::cross(d, r);
        for (int c = 0; c < 8; c++) {
            const glm::vec3 corner(c & 1 ? high.x : low.x, c & 2 ? high.y : low.y, c & 4 ? high.z : low.z);
            extent = std::max({extent, std::abs(glm::dot(corner - f.centre, r)), std::abs(glm::dot(corner - f.centre, u))});
        }
    }
    f.scale = f.half * FILL / extent;
    f.toViewer = viewerDirection(view.yawDegrees, view.pitchDegrees);
    f.right = rightOf(view.yawDegrees);
    f.up = glm::cross(f.toViewer, f.right);
    return f;
}

// A ray through a pixel of the picture, from in front of the box toward it, in grid units.
struct Hit {
    bool found = false;
    glm::ivec3 cell{0};   // the grid cell hit
    glm::ivec3 normal{0}; // the face it was entered by, pointing back toward the viewer
    glm::vec3 point{0};   // where, in grid units
};

Hit cast(const Grid &g, const Frame &f, glm::vec2 pixel) {
    Hit hit;
    // The ray in grid units (model voxels plus one).
    const glm::vec3 onPlane = f.centre + f.right * ((pixel.x - f.half) / f.scale) + f.up * ((f.half - pixel.y) / f.scale);
    const glm::vec3 dir = -f.toViewer;
    const glm::vec3 origin = onPlane + glm::vec3(1.0f) + f.toViewer * (float)(g.nx + g.ny + g.nz);
    // Where it enters the grid's box (the slabs), and by which face.
    float enter = -1e30f, leave = 1e30f;
    int enterAxis = 0;
    for (int a = 0; a < 3; a++) {
        const float size = (float)(a == 0 ? g.nx : a == 1 ? g.ny : g.nz);
        if (std::abs(dir[a]) < 1e-8f) {
            if (origin[a] < 0.0f || origin[a] > size)
                return hit;
            continue;
        }
        float t0 = (0.0f - origin[a]) / dir[a], t1 = (size - origin[a]) / dir[a];
        if (t0 > t1)
            std::swap(t0, t1);
        if (t0 > enter)
            enter = t0, enterAxis = a;
        leave = std::min(leave, t1);
    }
    if (enter > leave || leave < 0.0f)
        return hit;
    // Walk the cells along the ray (one cell boundary at a time).
    glm::vec3 p = origin + dir * enter;
    glm::ivec3 cell(glm::clamp((int)std::floor(p.x), 0, g.nx - 1), glm::clamp((int)std::floor(p.y), 0, g.ny - 1),
                    glm::clamp((int)std::floor(p.z), 0, g.nz - 1));
    glm::ivec3 step(0), normal(0);
    glm::vec3 next(1e30f), delta(1e30f);
    for (int a = 0; a < 3; a++) {
        if (dir[a] > 0)
            step[a] = 1, next[a] = enter + ((float)(cell[a] + 1) - p[a]) / dir[a], delta[a] = 1.0f / dir[a];
        else if (dir[a] < 0)
            step[a] = -1, next[a] = enter + ((float)cell[a] - p[a]) / dir[a], delta[a] = -1.0f / dir[a];
    }
    normal[enterAxis] = -step[enterAxis];
    float t = enter;
    while (g.inside(cell.x, cell.y, cell.z)) {
        if (g.kind[g.index(cell.x, cell.y, cell.z)] != Grid::EMPTY) {
            hit.found = true, hit.cell = cell, hit.normal = normal, hit.point = origin + dir * t;
            return hit;
        }
        const int a = next.x < next.y ? (next.x < next.z ? 0 : 2) : (next.y < next.z ? 1 : 2);
        t = next[a];
        next[a] += delta[a];
        cell[a] += step[a];
        normal = glm::ivec3(0);
        normal[a] = -step[a];
    }
    return hit;
}

// How bright a face is: lit from above, a little from the viewer's left.
float faceShade(const Frame &f, glm::ivec3 normal) {
    const glm::vec3 light = glm::normalize(f.up * 0.9f + f.toViewer * 0.5f - f.right * 0.3f);
    return 0.5f + 0.5f * std::max(0.0f, glm::dot(glm::vec3(normal), light));
}
constexpr float EDGE = 0.07f, EDGE_DARK = 0.82f; // voxel edges drawn a little darker

} // namespace

bool isMaterialCube(const Item &item) { return item[TYPE] == CUBE_TYPE && subtypeOf(item) == CUBE_SUBTYPE; }

std::vector<PaletteEntry> paletteOf(const std::vector<std::pair<Item, int>> &carried) {
    std::vector<PaletteEntry> out;
    for (const auto &[item, count] : carried) {
        if (!isMaterialCube(item) || count <= 0)
            continue;
        auto same = std::find_if(out.begin(), out.end(), [&](const PaletteEntry &e) { return nativeinteract::sameItem(e.item.data(), item.data()); });
        if (same == out.end())
            out.push_back({count, item});
        else
            same->count += count;
    }
    return out;
}

std::vector<PaletteEntry> paletteOf(const inventoryslots::Inventory &inventory, const Item &weapon) {
    if (weapon[TYPE] == 0)
        return {};
    std::array<Item, 5> candidates{};
    candidates[0] = nativesaves::makeItem(CUBE_TYPE, CUBE_SUBTYPE, weapon[MATERIAL]);
    for (int i = 1; i < 5; ++i)
        candidates[(size_t)i] = nativesaves::makeItem(CUBE_TYPE, 14, 127 + i);
    std::array<uint32_t, 5> totals{};
    for (const auto &tab : inventory.tabs)
        for (const auto &slot : tab) {
            if (nativeinteract::sameItem(candidates[0].data(), slot.item.data()))
                totals[0] += (uint32_t)slot.count;
            const uint8_t material = slot.item[MATERIAL];
            if (slot.item[TYPE] == CUBE_TYPE && subtypeOf(slot.item) == 14 && material >= 128 && material <= 131)
                totals[(size_t)material - 127] += (uint32_t)slot.count;
        }
    std::vector<PaletteEntry> out;
    for (size_t i = 0; i < candidates.size(); ++i)
        if (totals[i] != 0)
            out.push_back({std::bit_cast<int32_t>(totals[i]), candidates[i]});
    return out;
}

ClickResult click(Widget &w, uint8_t *weapon, bool overOtherWidget) {
    ClickResult result;
    // No voxel under the pointer, or no weapon: a carried cube let go outside the interface is
    // gone (its item is not given back); either way the selection and the carried cube end.
    if (!w.pointed || !weapon) {
        if (w.carrying && weapon && !overOtherWidget)
            result.weaponChanged = gamecraft::removeCube(weapon, w.carried.x, w.carried.y, w.carried.z);
        w.selected = -1;
        w.carrying = false;
        return result;
    }
    const Voxel at = *w.pointed;
    // Placing from the palette: the cube goes in (or recolours the one there) and one of its item
    // is taken. With the weapon's own cap reached the place routine refuses, but the item is
    // still taken (as the spec reads 0x292650; its open question 5).
    if (w.selectionValid()) {
        if (cubeCount(weapon) < WIDGET_LIMIT) {
            const PaletteEntry &e = w.palette[(size_t)w.selected];
            result.weaponChanged = gamecraft::placeCube(weapon, at.x, at.y, at.z, e.item[MATERIAL]);
            result.takeOne = true;
            result.taken = e.item;
        }
        return result;
    }
    // Moving a carried cube: from its old place to this one, free.
    if (w.carrying) {
        gamecraft::removeCube(weapon, w.carried.x, w.carried.y, w.carried.z);
        gamecraft::placeCube(weapon, at.x, at.y, at.z, w.carriedMaterial);
        w.carrying = false;
        result.weaponChanged = true;
        return result;
    }
    // Picking a placed cube up: nothing changes until it is put down.
    for (int i = 0; i < cubeCount(weapon); i++)
        if (cubeAt(weapon, i) == at) {
            w.carrying = true;
            w.carried = at;
            w.carriedMaterial = cubeMaterial(weapon, i);
            break;
        }
    return result;
}

// --- The weapon on screen -------------------------------------------------------------------

bool inReach(const CubModel &model, Voxel v) {
    return v.x >= -1 && v.y >= -1 && v.z >= -1 && v.x <= model.sx && v.y <= model.sy && v.z <= model.sz;
}

glm::vec4 cubeColour(uint8_t material) {
    const float white[4] = {1, 1, 1, 1};
    const std::array<float, 4> c = nativepose::materialColour(material, white, 0.0f);
    return {c[0], c[1], c[2], 1.0f};
}

std::optional<Voxel> pick(const CubModel &model, const uint8_t *weapon, const View &view, glm::vec2 pixel) {
    if (!model.sx || !model.sy || !model.sz)
        return std::nullopt;
    const Grid g = gridOf(model, weapon);
    const Hit hit = cast(g, frameOf(model, view), pixel);
    if (!hit.found)
        return std::nullopt;
    // A cube is pointed at itself; a voxel of the weapon by the empty cell in front of its face.
    glm::ivec3 cell = hit.cell - glm::ivec3(1);
    if (g.kind[g.index(hit.cell.x, hit.cell.y, hit.cell.z)] == Grid::WEAPON)
        cell += hit.normal;
    const Voxel v{(int8_t)cell.x, (int8_t)cell.y, (int8_t)cell.z};
    if (!inReach(model, v))
        return std::nullopt;
    return v;
}

glm::vec2 project(const CubModel &model, const View &view, Voxel v) {
    const Frame f = frameOf(model, view);
    const glm::vec3 middle = glm::vec3(v.x, v.y, v.z) + glm::vec3(0.5f) - f.centre;
    return {f.half + glm::dot(middle, f.right) * f.scale, f.half - glm::dot(middle, f.up) * f.scale};
}

float voxelPixels(const CubModel &model, const View &view) { return frameOf(model, view).scale; }

std::vector<uint8_t> render(const CubModel &model, const uint8_t *weapon, const View &view) {
    const int n = view.pixels;
    std::vector<uint8_t> rgba((size_t)n * n * 4, 0);
    if (!model.sx || !model.sy || !model.sz)
        return rgba;
    const Grid g = gridOf(model, weapon);
    const Frame f = frameOf(model, view);
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            const Hit hit = cast(g, f, glm::vec2((float)x + 0.5f, (float)y + 0.5f));
            if (!hit.found)
                continue;
            float shade = faceShade(f, hit.normal);
            // The point's place on its face: near an edge it is drawn darker.
            const glm::vec3 local = hit.point - glm::vec3(hit.cell);
            int edges = 0;
            for (int a = 0; a < 3; a++)
                if (hit.normal[a] == 0 && (local[a] < EDGE || local[a] > 1.0f - EDGE))
                    edges++;
            if (edges)
                shade *= EDGE_DARK;
            const glm::u8vec3 c = g.colour[g.index(hit.cell.x, hit.cell.y, hit.cell.z)];
            uint8_t *out = &rgba[((size_t)y * n + x) * 4];
            out[0] = (uint8_t)std::min(255.0f, c.r * shade), out[1] = (uint8_t)std::min(255.0f, c.g * shade);
            out[2] = (uint8_t)std::min(255.0f, c.b * shade), out[3] = 255;
        }
    return rgba;
}

// --- The window -----------------------------------------------------------------------------

namespace {
// Placed by CubeVulkan (the spec gives no geometry): right of the bag as the vendor's widget is
// (the bag is 400 units wide and its top 340 units above the middle), the picture under the
// titles, the turn arrows under it, the palette at the bottom.
constexpr float BESIDE_BAG_X = 210, ABOVE_MIDDLE = 340, MARGIN = 10;
constexpr glm::vec2 VIEW_OFFSET{22, 52}, ARROW_SIZE{12, 15};
constexpr float ARROW_GAP = 6, PALETTE_TOP = 338, PALETTE_LEFT = 15;
} // namespace

Layout layout(glm::vec2 window) {
    Layout l;
    l.at = glm::vec2(window.x * 0.5f + BESIDE_BAG_X, window.y * 0.5f - ABOVE_MIDDLE);
    l.at.x = std::min(l.at.x, window.x - WINDOW_SIZE.x - MARGIN);
    l.at.y = std::max(l.at.y, MARGIN);
    l.viewAt = l.at + VIEW_OFFSET;
    l.turnLeftAt = l.viewAt + glm::vec2(0, l.viewSize + ARROW_GAP);
    l.turnRightAt = l.viewAt + glm::vec2(l.viewSize - ARROW_SIZE.x, l.viewSize + ARROW_GAP);
    l.paletteAt = l.at + glm::vec2(PALETTE_LEFT, PALETTE_TOP);
    return l;
}

bool Layout::inWindow(glm::vec2 p) const { return screenitems::inside(p, at, WINDOW_SIZE); }
bool Layout::onView(glm::vec2 p) const { return screenitems::inside(p, viewAt, glm::vec2(viewSize)); }
glm::vec2 Layout::viewPixel(glm::vec2 p, int pixels) const { return (p - viewAt) * ((float)pixels / viewSize); }
int Layout::turnAt(glm::vec2 p) const {
    // (a little larger than the arrows, which are small)
    const glm::vec2 grow(6, 4);
    if (screenitems::inside(p, turnLeftAt - grow, ARROW_SIZE + grow * 2.0f))
        return -1;
    if (screenitems::inside(p, turnRightAt - grow, ARROW_SIZE + grow * 2.0f))
        return 1;
    return 0;
}
glm::vec2 Layout::paletteBox(int index) const {
    return paletteAt + glm::vec2((float)(index % PALETTE_COLUMNS) * (PALETTE_BOX + PALETTE_GAP),
                                 (float)(index / PALETTE_COLUMNS) * (PALETTE_BOX + PALETTE_GAP));
}
int Layout::paletteBoxAt(glm::vec2 p) const {
    for (int i = 0; i < PALETTE_COLUMNS * PALETTE_ROWS; i++)
        if (screenitems::inside(p, paletteBox(i), glm::vec2(PALETTE_BOX)))
            return i;
    return -1;
}

Hud::Screen screen(const Layout &l, const Shown &s) {
    using namespace screenitems;
    Hud::Screen out;
    out.push_back(square(l.at + glm::vec2(4), WINDOW_SIZE - glm::vec2(8), WINDOW_SHADE));
    out.push_back(widget("blackwidget", l.at, WINDOW_SIZE));
    out.push_back(text(s.title, l.at + glm::vec2(15, 25), TITLE_COLOUR));
    out.push_back(text(s.weaponName, l.at + glm::vec2(15, 42), s.weaponColour));
    if (!s.pictureIcon.empty())
        out.push_back(icon(s.pictureIcon, l.viewAt, l.viewSize));
    if (s.marker) {
        const glm::vec2 side(std::max(4.0f, s.markerSize));
        out.push_back(square(*s.marker - side * 0.5f, side, s.markerColour));
    }
    out.push_back(widget("leftbutton", l.turnLeftAt, glm::vec2(0), "", s.hoverTurn < 0 ? HOVER_COLOUR : glm::vec4(1)));
    out.push_back(widget("rightbutton", l.turnRightAt, glm::vec2(0), "", s.hoverTurn > 0 ? HOVER_COLOUR : glm::vec4(1)));
    const glm::vec4 count = s.cubes >= s.cap ? REFUSED_COLOUR : glm::vec4(1);
    out.push_back(text("Cubes " + std::to_string(s.cubes) + " / " + std::to_string(s.cap),
                       l.viewAt + glm::vec2(l.viewSize * 0.5f, l.viewSize + 18.0f), count, 1));
    const int shown = std::min((int)s.palette.size(), PALETTE_COLUMNS * PALETTE_ROWS);
    for (int i = 0; i < shown; i++) {
        const glm::vec2 at = l.paletteBox(i);
        const glm::vec4 tint = i == s.selected ? HOVER_COLOUR : i == s.hoverBox ? glm::vec4(0.75f, 1, 1, 1) : glm::vec4(1);
        out.push_back(widget("itembox", at, glm::vec2(PALETTE_BOX), "", tint));
        const Shown::Box &b = s.palette[(size_t)i];
        if (!b.icon.empty())
            out.push_back(icon(b.icon, at + glm::vec2(4), PALETTE_BOX - 8));
        else // (no model for it: a swatch of its material)
            out.push_back(square(at + glm::vec2(10), glm::vec2(PALETTE_BOX - 20), b.colour));
        out.push_back(text(std::to_string(b.count), at + glm::vec2(PALETTE_BOX - 3, PALETTE_BOX - 4), glm::vec4(1), 2));
    }
    if (!s.hint.empty())
        out.push_back(text(s.hint, l.at + glm::vec2(WINDOW_SIZE.x * 0.5f, WINDOW_SIZE.y - 12.0f), HINT_COLOUR, 1));
    return out;
}

} // namespace upgradewidget
