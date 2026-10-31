#include "ProceduralItems.h"
#include "NativeItems.h"
#include "worldgen/Noise.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <bit>
#include <cmath>
#include <utility>
#include <charconv>
#include <fstream>

namespace proceduralitem {
namespace {
using Color = glm::vec3;
using Point = glm::ivec3;
constexpr double PI = 3.141592653589793238462643383279502884;

class Builder {
  public:
    Resource resource;
    std::array<Color, 4> colors;
    uint32_t counter = 0;
    explicit Builder(Key key) {
        resource.key = key;
        resource.rgb.assign(3 * 24 * 24 * 24, 0);
        const auto parameters = nativeitems::makeOnDemandModel(key.id, key.modifier);
        counter = parameters.counter;
        for (size_t i = 0; i < colors.size(); ++i)
            colors[i] = {parameters.colours[i][0], parameters.colours[i][1], parameters.colours[i][2]};
    }
    uint32_t draw() {
        uint32_t value = counter++;
        value ^= value << 13;
        value ^= value >> 17;
        value ^= value << 5;
        return value;
    }
    Color palette(uint32_t low = 10, uint32_t range = 250) {
        const float blue = float(low + draw() % range);
        const float green = float(low + draw() % range);
        const float red = float(low + draw() % range);
        return {red, green, blue};
    }
    Color warm() {
        const float blue = float(10 + draw() % 100);
        const float green = float(120 + draw() % 130);
        const float red = float(180 + draw() % 70);
        return {red, green, blue};
    }
    std::array<float, 4> planes() {
        const float sum = float(draw() % 1000) / 1000.f;
        const float yz = float(draw() % 1000) / 1000.f;
        const float xz = float(draw() % 1000) / 1000.f;
        const float xy = float(draw() % 1000) / 1000.f;
        return {xy, xz, yz, sum};
    }
    static bool inside(Point p) { return p.x >= 0 && p.x < 24 && p.y >= 0 && p.y < 24 && p.z >= 0 && p.z < 24; }
    static size_t index(Point p) { return size_t((p.z * 24 + p.y) * 24 + p.x) * 3; }
    bool black(Point p) const {
        if (!inside(p))
            return true;
        const size_t at = index(p);
        return !(resource.rgb[at] | resource.rgb[at + 1] | resource.rgb[at + 2]);
    }
    void point(Point p, Color color, bool blackOnly = false) {
        if (!inside(p) || (blackOnly && !black(p)))
            return;
        const size_t at = index(p);
        for (int channel = 0; channel < 3; ++channel)
            resource.rgb[at + channel] = uint8_t(std::clamp(color[channel] * .5f, 0.f, 255.f));
    }
    void gradient(Point p, Color a, Color b, float fraction) { point(p, a * (1.f - fraction) + b * fraction); }
    void box(Point lower, Point extent, Color a, Color b) {
        if (extent.x <= 0 || extent.y <= 0 || extent.z <= 0)
            return;
        for (int x = lower.x; x < lower.x + extent.x; ++x)
            for (int y = lower.y; y < lower.y + extent.y; ++y)
                for (int z = lower.z; z < lower.z + extent.z; ++z)
                    gradient({x, y, z}, a, b, float(z - lower.z) / float(extent.z));
    }
    void sphere(Point center, int radius, Color a, Color b) {
        for (int x = center.x - radius; x <= center.x + radius; ++x)
            for (int y = center.y - radius; y <= center.y + radius; ++y)
                for (int z = center.z - radius; z <= center.z + radius; ++z) {
                    const float dx = float(x - center.x) - .5f, dy = float(y - center.y) - .5f, dz = float(z - center.z) - .5f;
                    if ((dz * dz + dx * dx) + dy * dy <= float(radius * radius))
                        gradient({x, y, z}, a, b, float(z - center.z + radius) / float(2 * radius));
                }
    }
    void faceted(Point center, int radius, Color a, Color b, const std::array<float, 4> &coefficients) {
        for (int x = center.x - radius; x <= center.x + radius; ++x)
            for (int y = center.y - radius; y <= center.y + radius; ++y)
                for (int z = center.z - radius; z <= center.z + radius; ++z) {
                    const float dx = float(std::abs(x - center.x)), dy = float(std::abs(y - center.y)), dz = float(std::abs(z - center.z));
                    if ((dx + dy) * coefficients[0] <= radius && (dx + dz) * coefficients[1] <= radius &&
                        (dy + dz) * coefficients[2] <= radius && ((dx + dy) + dz) * coefficients[3] <= radius)
                        gradient({x, y, z}, a, b, float(z - center.z + radius) / float(2 * radius));
                }
    }
    void extrudeXZ(int cx, int cz, int lowerY, int radius, int extent, Color a, Color b, bool upper, int innerRadius) {
        for (int x = cx - radius; x <= cx + radius; ++x)
            for (int z = upper ? cz : cz - radius; z <= cz + radius; ++z) {
                const float dx = float(x - cx) - .5f, dz = float(z - cz) - .5f;
                const float distance = dz * dz + dx * dx;
                if (distance > radius * radius || (innerRadius >= 0 && distance < innerRadius * innerRadius))
                    continue;
                for (int y = lowerY; y < lowerY + extent; ++y)
                    gradient({x, y, z}, a, b, float(z - cz + radius) / float(2 * radius));
            }
    }
    void ringXZ(int cx, int cz, int lowerY, int radius, int extent, int thickness, Color a, Color b) {
        extrudeXZ(cx, cz, lowerY, radius, extent, a, b, false, radius - thickness);
    }
    void discXZ(int cx, int cz, int lowerY, int radius, int extent, Color a, Color b, bool upper = false) {
        extrudeXZ(cx, cz, lowerY, radius, extent, a, b, upper, -1);
    }
    void upperDiscYZ(int cy, int cz, int lowerX, int radius, int extent, Color a, Color b) {
        for (int y = cy - radius; y <= cy + radius; ++y)
            for (int z = cz; z <= cz + radius; ++z) {
                const float dy = float(y - cy) - .5f, dz = float(z - cz) - .5f;
                if (dz * dz + dy * dy <= radius * radius)
                    for (int x = lowerX; x < lowerX + extent; ++x)
                        gradient({x, y, z}, a, b, float(z - cz + radius) / float(2 * radius));
            }
    }
    void discXY(int cx, int cy, int lowerZ, int radius, int extent, Color a, Color b) {
        for (int x = cx - radius; x <= cx + radius; ++x)
            for (int y = cy - radius; y <= cy + radius; ++y) {
                const float dx = float(x - cx) - .5f, dy = float(y - cy) - .5f;
                if (dy * dy + dx * dx <= radius * radius)
                    for (int z = lowerZ; z < lowerZ + extent; ++z)
                        gradient({x, y, z}, a, b, float(z - lowerZ) / float(extent));
            }
    }
    static float ornamentAngle(int index, int count) {
        const float n = float(count);
        const float fraction = (float(index) - (n + n)) / n;
        const float doubled = fraction + fraction;
        return float(double(doubled) * PI + PI / 2);
    }
    void rectangular() {
        const Point extent{6 + int(draw() % 12), 6 + int(draw() % 12), 6 + int(draw() % 12)};
        box((Point(24) - extent) / 2, extent, colors[0], colors[1]);
    }
    void fallback() {
        const float blue = float(draw() & 127), green = float(draw() & 127), red = float(draw() & 127);
        const Color a{red, green, blue};
        sphere({12, 12, 12}, 8, a, a * .5f);
    }
    void crystal() { faceted({12, 12, 12}, 8, colors[0], colors[0] * .5f, planes()); }
    void staff() {
        Color a = warm(), b = warm();
        const int radius = 3 + int(draw() % 3), height = 23 - 2 * radius;
        box({11, 11, 0}, {3, 3, height}, a, b);
        const int collars = 1 + int(draw() % 3);
        for (int i = 0; i < collars; ++i) {
            const float fraction = float(i) / float(collars), inverse = 1.f - fraction;
            const Color color = b * inverse + a * fraction;
            box({10, 10, int(float(height) * inverse)}, {5, 5, 1}, color, color);
        }
        a = palette();
        palette(); // The unused second head palette still advances three draws.
        faceted({12, 12, height + radius}, radius, a, a * .5f, planes());
    }
    void crown() {
        const int radius = 9 - int(draw() % 3), count = 2 + 2 * int(draw() % 3), edge = 2 + int(draw() & 1);
        const Color ornament = palette();
        for (int i = 0; i < count; ++i) {
            const float angle = ornamentAngle(i, count);
            const float x = (std::cos(angle) * float(radius + 1) + 12.f) + 1.f;
            const float y = std::sin(angle) * float(radius + 1) + 13.f;
            const Point lower{int(x) - edge / 2, int(y) - edge / 2, 8};
            box(lower, Point(edge), ornament, ornament * .5f);
            point(lower + Point(edge - 1), ornament + 150.f);
        }
        const Color a = warm(), b = warm();
        const int period = 2 + int(draw() % 6);
        for (int x = 12 - radius; x <= 12 + radius; ++x)
            for (int y = 12 - radius; y <= 12 + radius; ++y) {
                const float dx = float(x - 12) - .5f, dy = float(y - 12) - .5f;
                const float distance = dy * dy + dx * dx;
                if (distance > radius * radius || distance < (radius - 1) * (radius - 1))
                    continue;
                const int xDistance = std::abs(period - (x + 12 - period) % (2 * period));
                const int yDistance = std::abs(period - (y + 12 - period) % (2 * period));
                const int end = (8 + 2 * period) - yDistance - xDistance;
                for (int z = 6; z < end; ++z)
                    gradient({x, y, z}, a, b, float(z - 6) / float(end - 6));
            }
    }
    void ring() {
        const Color a = colors[0], c = colors[2], metal = palette(100, 70);
        const int radius = 6 + int(draw() % 4), width = 1 + int(draw() % 5), thickness = 1 + int(draw() % 3);
        const int style = int(draw() & 3), lower = 12 - width / 2;
        int effective = radius;
        auto band = [&](int r, int y, int w, int t, Color color) { ringXZ(12, 12, y, r, w, t, color, color * .75f); };
        if (style == 0)
            band(radius, lower, width, thickness, metal);
        else if (style == 1 || style == 3) {
            effective = radius - 1;
            band(effective, lower, width, thickness, c);
            if (style == 3) {
                band(effective, lower - 2, 1, thickness, metal);
                band(radius, lower - 1, 1, thickness + 1, metal);
                band(radius, lower + width, 1, thickness + 1, metal);
                band(effective, lower + width + 1, 1, thickness, metal);
            } else {
                band(radius, lower - 2, 2, thickness + 1, metal);
                band(radius, lower + width, 2, thickness + 1, metal);
            }
        } else {
            band(radius + 1, lower, width, thickness + 1, metal);
            band(radius, lower - 2, 2, thickness + 1, metal);
            band(radius, lower + width, 2, thickness + 1, metal);
        }
        if (!(draw() & 1)) {
            const int settingRadius = width / 2 + 1 + int(draw() & 1), height = 1 + int(draw() & 1);
            if (draw() & 1) {
                box({12 - settingRadius, 11 - settingRadius, 13 + effective}, Point(2 * settingRadius + 2, 2 * settingRadius + 2, 1), metal,
                    metal);
                box({13 - settingRadius, 12 - settingRadius, 14 + effective}, {2 * settingRadius, 2 * settingRadius, height}, a, a);
                point({12 + settingRadius, 11 + settingRadius, 13 + effective + height}, a + 150.f);
            } else {
                discXY(12, 12, 13 + effective, settingRadius + 1, 1, metal, metal);
                discXY(12, 12, 14 + effective, settingRadius, height, a, a);
                const int position = int(13.f - float(settingRadius) * .5f);
                point({position, position, 13 + effective + height}, a + 150.f);
            }
        } else {
            const int count = 2 + int(draw() & 3), edge = 2 + int(draw() % uint32_t(width));
            for (int i = 0; i < count; ++i) {
                const float angle = ornamentAngle(i, count);
                const Point lower{int(std::cos(angle) * float(effective + 1) + 13.5f) - edge / 2, 12 - edge / 2,
                                  int(std::sin(angle) * float(effective + 1) + 13.5f) - edge / 2};
                box(lower, Point(edge), a, a * .5f);
                point(lower + Point(edge - 1), a + 150.f);
            }
        }
    }
    void pendant() {
        const int k = int(draw() % 3), radius = k + 3, center = k + 5;
        const bool crossed = (draw() & 1) != 0;
        const Color a = colors[0], metal = palette(100, 50);
        const int style = int(draw() & 3), layer = crossed ? 12 : 13;
        if (style == 1) {
            if (crossed) {
                discXZ(12, center, 12, radius + 1, 2, metal, metal, true);
                upperDiscYZ(12, center, 12, radius + 1, 2, metal, metal);
            } else
                sphere({12, 12, center + radius}, radius / 2, metal, metal);
            sphere({12, 12, center}, radius, a, a * .5f);
        } else if (style == 2) {
            if (crossed) {
                box({11 - radius, 11, center}, {2 * radius + 2, 2, radius + 1}, metal, metal);
                box({11, 11 - radius, center}, {2, 2 * radius + 2, radius + 1}, metal, metal);
            } else {
                const int q = 3 * radius / 4;
                box({13 - q, 13 - q, center + radius + 2}, {2 * q - 2, 2 * q - 2, 1}, metal, metal);
                box({12 - q, 12 - q, center + radius + 1}, {2 * q, 2 * q, 1}, metal, metal);
            }
            box({12 - radius, 12 - radius, 2}, Point(2 * radius), a, a * .5f);
        } else {
            if (style == 3) {
                if (crossed)
                    box({11 - radius, 12, center}, {2 * radius + 2, 1, radius + 1}, metal, metal);
                else
                    box({11 - radius, 11, 1}, {2 * radius + 2, 2, 2 * radius + 2}, metal, metal);
                box({12 - radius, layer, 2}, {2 * radius, 1, 2 * radius}, a, a * .5f);
            } else {
                discXZ(12, center, crossed ? 12 : 11, radius + 1, crossed ? 1 : 2, metal, metal, crossed);
                discXZ(12, center, layer, radius, 1, a, a * .5f);
            }
            point({10 + radius, layer, 2 * radius}, a + 150.f);
        }
        const int start = center + radius, remaining = 24 - start, count = 4 * remaining;
        const float spread = float(remaining) / 3.f;
        const int32_t modifier = std::bit_cast<int32_t>(resource.key.modifier);
        const int32_t modifier13 = std::bit_cast<int32_t>(resource.key.modifier * 13u);
        for (int i = 0; i < count; ++i) {
            const float t = float(i) / float(count - 1), u = t * t, z = float(i) * .25f + float(start);
            const double difference = double(u) - .5;
            const float v = float(std::abs(difference + difference)), q = 1.f - v * v;
            const float noise =
                worldgen::simplex((double(q) * .25 + double(modifier)) + 8741, (double(modifier13) + double(z) * .25) + 32992);
            const float span = q * spread;
            float jitter = (u * .5f) * spread;
            jitter *= noise;
            point({int((12.5f + span) + jitter), 12, int(z)}, metal);
            point({int((12.5f - span) + jitter), 12, int(z)}, metal);
        }
    }
    static Color shiftedHue(Color a) {
        a /= 255.f;
        const float minimum = std::min({a.r, a.g, a.b}), maximum = std::max({a.r, a.g, a.b});
        const double delta = double(maximum) - double(minimum);
        float hue = 0, saturation = 0;
        if (delta >= 1e-5) {
            saturation = float(delta / double(maximum));
            const float numerator = a.r == maximum ? a.g - a.b : a.g == maximum ? a.b - a.r : a.r - a.g;
            const double offset = a.r == maximum ? 0.0 : a.g == maximum ? 2.0 : 4.0;
            hue = float(double(float(double(numerator) / delta + offset)) * 60.0);
            if (hue < 0)
                hue = float(double(hue) + 360.0);
        }
        hue += 180.f;
        if (saturation <= 0)
            return Color(maximum) * 255.f;
        const double h = (hue >= 360.f ? 0.0 : double(hue)) / 60.0;
        const int sector = int(h);
        const double fraction = h - sector, s = saturation, value = maximum;
        const float low = float((1.0 - s) * value), falling = float((1.0 - s * fraction) * value);
        const float rising = float((1.0 - (1.0 - fraction) * s) * value);
        Color result;
        switch (sector) {
        case 0:
            result = {maximum, rising, low};
            break;
        case 1:
            result = {falling, maximum, low};
            break;
        case 2:
            result = {low, maximum, rising};
            break;
        case 3:
            result = {low, falling, maximum};
            break;
        case 4:
            result = {rising, low, maximum};
            break;
        default:
            result = {maximum, low, falling};
            break;
        }
        return result * 255.f;
    }
    void vessel() {
        const float r0 = float(8 - draw() % 6), r2 = float(8 - draw() % 6);
        const int height = 24 - int(draw() % 12), profile = int(draw() % 3), pattern = int(draw() & 3);
        const Color a = palette(), b = a + 30.f, c = shiftedHue(a), d = colors[3];
        const int handles = int(draw() % 3), handleStart = int(draw() % uint32_t(height / 4 + 1));
        const bool topBand = (draw() & 1) != 0, bottomBand = (draw() & 1) != 0, alternateHandle = (draw() & 1) != 0;
        const int32_t modifier = std::bit_cast<int32_t>(resource.key.modifier);
        for (int z = 0; z < height; ++z) {
            const float t = float(z) / float(height - 1);
            const Color base = a * (1.f - t) + b * t;
            const float p = profile == 1 ? t * t : profile == 2 ? 1.f - (1.f - t) * (1.f - t) : t, q = 1.f - p;
            const float radius = (q * (r0 * .5f) + p * r0) * q + (q * r0 + p * r2) * p;
            for (int x = int((12.f - radius) - 2.f); float(x) <= (12.f + radius) + 2.f; ++x)
                for (int y = int((12.f - radius) - 2.f); float(y) <= (radius + 2.f) + 12.f; ++y) {
                    const float dx = (float(x) - 12.f) + .5f, dy = (float(y) - 12.f) + .5f;
                    const float distance = dy * dy + dx * dx;
                    if (distance > radius * radius || (z > 0 && distance < (radius - 1.f) * (radius - 1.f)))
                        continue;
                    Color color = base;
                    if ((pattern == 1 && (z / 2) % 3 == 0) || (pattern == 2 && ((z / 4 + y / 2 + x / 2) & 1) == 0 && !(z & 2)))
                        color = c;
                    else if (pattern == 3) {
                        const float n1 =
                            worldgen::simplex((double(y) * .3 + 155) + double(modifier), (double(z) * .5 + 1003) + double(modifier));
                        const float n2 =
                            worldgen::simplex((double(x) * .3 + 3411) + double(modifier), (double(z) * .5 + 2131) + double(modifier));
                        float weight = std::clamp(n1 + n2, 0.f, 1.f);
                        weight = (weight * weight) * weight;
                        color = base + (c - base) * weight;
                    }
                    if (topBand && z >= height - 2)
                        color = d;
                    if (bottomBand && z < 2)
                        color = d;
                    point({x, y, z}, color);
                }
            if (handles && z >= handleStart) {
                const float fraction = float(z - handleStart) / float(height - 1 - handleStart);
                const float g = (.5f - fraction) + (.5f - fraction), bulge = (1.f - g * g) * 5.f;
                const int right = int(bulge + ((12.f + .5f) + radius));
                const Color color = alternateHandle ? d : base;
                for (int y : {11, 12}) {
                    point({right, y, z}, color, true);
                    point({right - 1, y, z}, color, true);
                }
                if (handles == 2) {
                    const int left = 24 - right;
                    for (int y : {11, 12}) {
                        point({left - 1, y, z}, color, true);
                        point({left, y, z}, color, true);
                    }
                }
            }
        }
    }
};
} // namespace
Resource generate(Key key) {
    Builder builder(key);
    switch (key.id) {
    case 0:
        builder.vessel();
        break;
    case 1:
        builder.pendant();
        break;
    case 2:
        builder.ring();
        break;
    case 3:
        builder.rectangular();
        break;
    case 4:
        builder.crystal();
        break;
    case 5:
        builder.crown();
        break;
    case 6:
        builder.staff();
        break;
    default:
        builder.fallback();
        break;
    }
    return std::move(builder.resource);
}
std::string spriteKey(Key key) {
    return "procedural/" + fileName(key);
}
std::optional<Key> parseSpriteKey(std::string_view text) {
    constexpr std::string_view prefix = "procedural/p_";
    if (!text.starts_with(prefix))
        return std::nullopt;
    text.remove_prefix(prefix.size());
    if (text.size() != 21 || text[8] != '_' || text.substr(17) != ".cwp")
        return std::nullopt;
    uint32_t id, modifier;
    const auto a = std::from_chars(text.data(), text.data() + 8, id, 16);
    const auto b = std::from_chars(text.data() + 9, text.data() + 17, modifier, 16);
    if (a.ec != std::errc{} || a.ptr != text.data() + 8 || b.ec != std::errc{} || b.ptr != text.data() + 17)
        return std::nullopt;
    return Key{std::bit_cast<int32_t>(id), modifier};
}
const Resource &OwnerCache::get(Key key, std::string *diagnostic) {
    if (diagnostic)
        diagnostic->clear();
    if (const auto found = resources.find(key); found != resources.end())
        return found->second;
    const auto file = directory / fileName(key);
    Resource resource;
    std::ifstream input(file, std::ios::binary | std::ios::ate);
    if (input) {
        const auto length = input.tellg();
        if (length == std::streamoff(HEADER_SIZE + 3 * 24 * 24 * 24)) {
            std::vector<uint8_t> bytes(size_t(length), uint8_t{});
            input.seekg(0);
            input.read(reinterpret_cast<char *>(bytes.data()), std::streamsize(bytes.size()));
            if (input && decode(bytes, resource, diagnostic) && resource.key == key &&
                resource.dimensions == std::array<int32_t, 3>{24, 24, 24} && resource.classification == 1 && resource.mode == 1 &&
                resource.radius == 3 && resource.shade == std::array<uint8_t, 3>{})
                return resources.emplace(key, std::move(resource)).first->second;
        }
        if (diagnostic && diagnostic->empty())
            *diagnostic = "Owner procedural resource has unsupported data or metadata: " + file.string();
    }
    return resources.emplace(key, generate(key)).first->second;
}
} // namespace proceduralitem
