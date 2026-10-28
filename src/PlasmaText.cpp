#include "PlasmaText.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_STROKER_H

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>

namespace plasma {

namespace {

constexpr float MAX_RASTER_SIZE = 1000;   // scalable raster sizes stay within 1 to this
constexpr float MAX_RASTER_RADIUS = 500;  // and outline radii within this
constexpr float MAX_GLYPH_SIDE = 500;     // a pixel mode glyph larger than this squared is shrunk
constexpr float FIXED_16_16 = 65536;
constexpr float FIXED_26_6 = 64;
// FreeType load flags: the pixel mode loads unhinted outlines, the scalable mode hinted ones.
constexpr FT_Int32 PIXEL_LOAD = FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP;
constexpr FT_Int32 SCALABLE_LOAD = FT_LOAD_NO_BITMAP;
constexpr wchar_t NEWLINE = 10, RETURN = 13, SPACE = 32;

int roundHalfUp(float v) { return (int)(v + 0.5f); } // (truncating, as the game rounds)

// A raster size in 26.6 fixed point.
FT_F26Dot6 fixed26(float v) { return (FT_F26Dot6)roundHalfUp(v * FIXED_26_6); }

} // namespace

struct Font::Face {
    std::vector<unsigned char> data;
    FT_Library library = nullptr;
    FT_Face face = nullptr;
    FT_Stroker stroker = nullptr;
    mutable std::map<std::tuple<std::array<int, 8>, bool, wchar_t>, Glyph> glyphs;

    ~Face() {
        if (stroker)
            FT_Stroker_Done(stroker);
        if (face)
            FT_Done_Face(face);
        if (library)
            FT_Done_FreeType(library);
    }

    // The char size and transform of a raster (spec 6.2); `shrink` scales the transform down.
    void select(const Raster &r, float shrink) const {
        if (r.scalable) {
            FT_Set_Char_Size(face, fixed26(r.size.x), fixed26(r.size.y), 0, 0);
            FT_Set_Transform(face, nullptr, nullptr);
            return;
        }
        FT_Set_Char_Size(face, fixed26(r.size.x), fixed26(r.size.x), 0, 0);
        // FreeType's y axis points up: the off-diagonal terms change sign.
        FT_Matrix m;
        m.xx = (FT_Fixed)(r.transform[0][0] * shrink * FIXED_16_16);
        m.xy = (FT_Fixed)(-r.transform[1][0] * shrink * FIXED_16_16);
        m.yx = (FT_Fixed)(-r.transform[0][1] * shrink * FIXED_16_16);
        m.yy = (FT_Fixed)(r.transform[1][1] * shrink * FIXED_16_16);
        FT_Set_Transform(face, &m, nullptr);
    }

    Glyph rasterize(wchar_t c, const Raster &r, bool outline) const {
        Glyph g;
        const FT_Int32 flags = r.scalable ? SCALABLE_LOAD : PIXEL_LOAD;
        float shrink = 1; // g in the spec: how much an oversized glyph was shrunk
        select(r, shrink);
        if (FT_Load_Char(face, (FT_ULong)c, flags))
            return g;
        if (!r.scalable) {
            // The oversize guard: the metric box through the transform.
            const float w = (float)face->glyph->metrics.width / FIXED_26_6, h = (float)face->glyph->metrics.height / FIXED_26_6;
            const glm::vec2 across = r.transform * glm::vec2(w, 0), down = r.transform * glm::vec2(0, h);
            const glm::vec2 extent = glm::abs(across) + glm::abs(down);
            const float area = extent.x * extent.y;
            if (area > MAX_GLYPH_SIDE * MAX_GLYPH_SIDE) {
                shrink = MAX_GLYPH_SIDE / std::sqrt(area);
                select(r, shrink);
                if (FT_Load_Char(face, (FT_ULong)c, flags))
                    return g;
            }
        }
        FT_Glyph glyph = nullptr;
        if (FT_Get_Glyph(face->glyph, &glyph))
            return g;
        g.advance = r.scalable ? (float)glyph->advance.x / FIXED_16_16 : (float)face->glyph->metrics.horiAdvance / FIXED_26_6;
        if (outline) {
            FT_Stroker_Set(stroker, (FT_Fixed)roundHalfUp(r.stroke * FIXED_26_6 * shrink), FT_STROKER_LINECAP_ROUND,
                           FT_STROKER_LINEJOIN_ROUND, 0);
            FT_Glyph_Stroke(&glyph, stroker, 1);
        }
        if (FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, nullptr, 1)) {
            FT_Done_Glyph(glyph);
            return g;
        }
        const FT_BitmapGlyph bitmap = reinterpret_cast<FT_BitmapGlyph>(glyph);
        const FT_Bitmap &b = bitmap->bitmap;
        g.exists = true;
        g.size = glm::ivec2((int)b.width + 2, (int)b.rows + 2);
        g.alpha.assign((size_t)g.size.x * (size_t)g.size.y, 0);
        for (unsigned y = 0; y < b.rows; y++)
            for (unsigned x = 0; x < b.width; x++)
                g.alpha[(size_t)(y + 1) * (size_t)g.size.x + x + 1] = b.buffer[(ptrdiff_t)y * b.pitch + x];
        g.offset = glm::vec2((float)(bitmap->left - 1), (float)(-bitmap->top - 1)) / shrink;
        g.extent = glm::vec2(g.size) / shrink;
        FT_Done_Glyph(glyph);
        return g;
    }
};

Font::Font() = default;
Font::~Font() = default;

bool Font::load(std::vector<unsigned char> bytes) {
    face.reset();
    if (bytes.empty())
        return false;
    auto f = std::make_unique<Face>();
    f->data = std::move(bytes);
    if (FT_Init_FreeType(&f->library) ||
        FT_New_Memory_Face(f->library, f->data.data(), (FT_Long)f->data.size(), 0, &f->face) ||
        FT_Stroker_New(f->library, &f->stroker))
        return false;
    face = std::move(f);
    return true;
}

bool Font::loaded() const { return face != nullptr; }

const Glyph &Font::glyph(wchar_t c, const Raster &raster, bool outline) const {
    static const Glyph none;
    if (!face)
        return none;
    const auto key = std::make_tuple(raster.key(), outline, c);
    auto it = face->glyphs.find(key);
    if (it == face->glyphs.end())
        it = face->glyphs.emplace(key, face->rasterize(c, raster, outline)).first;
    return it->second;
}

float Font::kerning(wchar_t a, wchar_t b, const Raster &raster) const {
    if (!face || !FT_HAS_KERNING(face->face))
        return 0;
    const FT_UInt left = FT_Get_Char_Index(face->face, (FT_ULong)a), right = FT_Get_Char_Index(face->face, (FT_ULong)b);
    FT_Vector v{};
    if (raster.scalable) {
        face->select(raster, 1);
        FT_Get_Kerning(face->face, left, right, FT_KERNING_DEFAULT, &v);
        return (float)(v.x >> 6);
    }
    FT_Get_Kerning(face->face, left, right, FT_KERNING_UNSCALED, &v);
    return (float)v.x * raster.size.x / (float)face->face->units_per_EM;
}

std::array<int, 8> Raster::key() const {
    return {scalable ? 1 : 0,
            (int)fixed26(size.x),
            (int)fixed26(size.y),
            (int)fixed26(stroke),
            (int)std::lround(transform[0][0] * FIXED_16_16),
            (int)std::lround(transform[0][1] * FIXED_16_16),
            (int)std::lround(transform[1][0] * FIXED_16_16),
            (int)std::lround(transform[1][1] * FIXED_16_16)};
}

Raster rasterFor(const glm::mat4 &m, const TextStyle &style) {
    Raster r;
    r.scalable = style.scalable;
    const float s = style.pixelSize;
    // The lengths of the matrix's x and y axes.
    const float sx = glm::length(glm::vec2(m[0])), sy = glm::length(glm::vec2(m[1]));
    if (!r.scalable) {
        r.size = glm::vec2(s);
        r.transform = glm::mat2(m);
        r.stroke = style.strokeRadius * std::min(sx, sy);
        r.spacing = style.spacing;
        r.leading = style.lineSpacing;
        r.wrapWidth = style.wrapWidth;
        return r;
    }
    auto rasterSize = [&](float scale) {
        return std::clamp((float)roundHalfUp(scale * s * FIXED_26_6) / FIXED_26_6, 1.0f, MAX_RASTER_SIZE);
    };
    r.size = glm::vec2(rasterSize(sx), rasterSize(sy));
    const glm::vec2 ratio = s > 0.0f ? r.size / s : glm::vec2(1); // raster pixels per text unit
    r.stroke = std::min((ratio.x + ratio.y) * 0.5f * style.strokeRadius, MAX_RASTER_RADIUS);
    r.spacing = style.spacing * ratio.x;
    r.leading = style.lineSpacing * ratio.y;
    r.wrapWidth = style.wrapWidth * ratio.x;
    return r;
}

glm::mat4 drawMatrix(const glm::mat4 &m, const TextStyle &style, const Raster &r) {
    glm::mat4 d(1);
    if (r.scalable) {
        d = m;
        d[0] *= style.pixelSize / r.size.x;
        d[1] *= style.pixelSize / r.size.y;
    }
    d[3].x = (float)(int)(m[3].x + 0.5f);
    d[3].y = (float)(int)(m[3].y + 0.5f);
    return d;
}

float nextPen(float pen, float advance, float kerning, float spacing, bool last, bool scalable) {
    // spec: open question - whether the spacing joins the advance before the scalable mode's cut
    // to whole pixels; it is taken to (one sum, cut once).
    if (!last)
        advance += kerning + spacing;
    return scalable ? (float)(int)(pen + advance) : pen + advance;
}

namespace {

float advanceOf(const Font &font, wchar_t c, const Raster &r) {
    const Glyph &g = font.glyph(c, r, false);
    return g.exists ? g.advance : 0.0f;
}

// The pen after character i of a text; `stopAtNewline` (measuring) also leaves out the kerning
// and spacing before a newline.
float penAfter(const Font &font, const std::wstring &text, size_t i, float pen, const Raster &r, bool stopAtNewline) {
    const bool last = i + 1 >= text.size() || (stopAtNewline && text[i + 1] == NEWLINE);
    const float kerning = last ? 0.0f : font.kerning(text[i], text[i + 1], r);
    return nextPen(pen, advanceOf(font, text[i], r), kerning, r.spacing, last, r.scalable);
}

} // namespace

TextBox measure(const Font &font, const std::wstring &text, const Raster &r, int flags, int start, bool inkBox) {
    TextBox box{glm::vec2(0, -r.size.y), glm::vec2(0)};
    glm::vec2 pen(0);
    bool firstInk = true;
    for (size_t i = start < 0 ? 0 : (size_t)start; i < text.size(); i++) {
        const wchar_t c = text[i];
        if (c == 0 || (start >= 0 && c == NEWLINE))
            break;
        if (!inkBox)
            box.maximum = glm::max(box.maximum, pen);
        else if (c != NEWLINE && c != RETURN) {
            // (the outline glyph when there is an outline: it is the larger)
            const Glyph &g = font.glyph(c, r, r.stroke > 0.0f);
            const glm::vec2 lo = pen + g.offset + 1.0f, hi = pen + g.offset + g.extent - 1.0f;
            if (firstInk)
                box = {lo, hi}, firstInk = false;
            else
                box.minimum = glm::min(box.minimum, lo), box.maximum = glm::max(box.maximum, hi);
        }
        if (c == NEWLINE)
            pen = glm::vec2(0, pen.y + r.size.y + r.leading);
        else if (c != RETURN)
            pen.x = penAfter(font, text, i, pen.x, r, true);
    }
    if (!inkBox)
        box.maximum = glm::max(box.maximum, pen); // (one place past the end)
    if (flags & (TEXT_CENTER | TEXT_RIGHT | TEXT_MIDDLE | TEXT_BOTTOM)) {
        const TextBox whole = measure(font, text, r, 0, -1, inkBox);
        glm::vec2 shift(0);
        if (flags & TEXT_CENTER)
            shift.x = -(whole.minimum.x + whole.maximum.x) * 0.5f;
        if (flags & TEXT_RIGHT)
            shift.x = -whole.maximum.x;
        if (flags & TEXT_MIDDLE)
            shift.y = -(whole.minimum.y + whole.maximum.y) * 0.5f;
        if (flags & TEXT_BOTTOM)
            shift.y = -whole.maximum.y;
        box.minimum += shift, box.maximum += shift;
    }
    return box;
}

std::wstring wrapText(const Font &font, std::wstring text, const Raster &r) {
    float line = 0, atBreak = 0;
    int lastBreak = -1;
    for (size_t i = 0; i < text.size(); i++) {
        const wchar_t c = text[i];
        if (c == SPACE || c == NEWLINE || c == RETURN || i + 1 == text.size()) {
            if (line >= r.wrapWidth && lastBreak >= 0) {
                text[(size_t)lastBreak] = NEWLINE;
                line -= atBreak; // (the space that became the break stays counted)
            }
            atBreak = line;
            lastBreak = (int)i;
        }
        if (c == NEWLINE || c == RETURN)
            line = atBreak = 0;
        else
            line = penAfter(font, text, i, line, r, false);
    }
    return text;
}

Layout layout(const Font &font, const std::wstring &source, const Raster &r, int flags) {
    const std::wstring text = flags & TEXT_WRAP ? wrapText(font, source, r) : source;
    Layout out;
    // The block's shift: how far the alignment moves the pens' box.
    const glm::vec2 shift = measure(font, text, r, flags, -1, false).minimum - measure(font, text, r, 0, -1, false).minimum;
    // Each line is aligned on its own by its width.
    auto lineWidth = [&](size_t start) {
        const TextBox line = measure(font, text, r, 0, (int)start, false);
        return line.maximum.x - line.minimum.x;
    };
    auto alignLine = [&](float width) {
        if (flags & TEXT_CENTER)
            out.pen.x = -width * 0.5f;
        if (flags & TEXT_RIGHT)
            out.pen.x = -width;
    };
    out.pen = shift;
    alignLine(lineWidth(0));
    for (size_t i = 0; i < text.size(); i++) {
        const wchar_t c = text[i];
        if (c == NEWLINE) {
            out.pen.x = shift.x;
            alignLine(lineWidth(i + 1));
            out.pen.y += r.size.y + r.leading;
            continue;
        }
        out.glyphs.push_back({c, out.pen});
        out.pen.x = penAfter(font, text, i, out.pen.x, r, false);
    }
    return out;
}

glm::vec2 glyphPosition(const Raster &r, glm::vec2 pen, glm::vec2 offset) {
    if (r.scalable)
        return glm::vec2((float)(int)pen.x + (float)(int)offset.x, (float)(int)pen.y + (float)(int)offset.y);
    const glm::vec2 p = r.transform * pen;
    return glm::vec2((float)(int)(p.x + offset.x), (float)(int)(p.y + offset.y));
}

bool textBounds(const Font &font, const TextStyle &style, const glm::mat4 &matrix, TextBox &out) {
    if (!font.loaded() || !style.scalable || style.pixelSize <= 0.0f)
        return false;
    const Raster r = rasterFor(matrix, style);
    out = measure(font, style.string, r, style.flags, -1, true);
    const glm::vec2 back = style.pixelSize / r.size;
    out.minimum *= back, out.maximum *= back;
    return true;
}

bool textStyle(const plx::Object &shape, int loaderFlags, TextStyle &out) {
    if (shape.kind != "TextShape")
        return false;
    out = TextStyle{};
    out.scalable = (loaderFlags & plx::LOAD_SCALABLE_TEXT) != 0;
    for (const plx::Chunk &p : shape.properties) {
        const std::string &n = p.name;
        if (n == "TextShape.flags")
            out.flags = p.as<int32_t>();
        else if (n == "TextShape.pixelSize")
            out.pixelSize = p.as<float>();
        else if (n == "TextShape.fontSize") // (an older name, in whole pixels)
            out.pixelSize = (float)p.as<int32_t>();
        else if (n == "TextShape.strokeRadius")
            out.strokeRadius = p.as<float>();
        else if (n == "TextShape.strokeWidth") // (an older name: a full width)
            out.strokeRadius = (float)p.as<int32_t>() * 0.5f;
        else if (n == "TextShape.spacing")
            out.spacing = p.as<float>();
        else if (n == "TextShape.lineSpacing")
            out.lineSpacing = p.as<float>();
        else if (n == "TextShape.wrapWidth")
            out.wrapWidth = p.as<float>();
        else if (n == "TextShape.fontName" || n == "TextShape.wfontName")
            out.font = p.text();
        else if (n == "TextShape.string") {
            const auto slots = p.frames();
            if (!slots.empty())
                out.string = plx::wideString(*slots[0]);
        } else if (n == "TextShape.color")
            out.color = plx::live<glm::vec4>(&p, out.color);
        else if (n == "TextShape.strokeColor")
            out.strokeColor = plx::live<glm::vec4>(&p, out.strokeColor);
        else if (n == "TextShape.extrusionColor")
            out.extrusionColor = plx::live<glm::vec4>(&p, out.extrusionColor);
        else if (n == "TextShape.font") { // the older font block: size and radius both written at its end
            float size = 0, radius = 0;
            for (const plx::Chunk &f : p.children) {
                if (f.name == "TextShape.font.fileName" || f.name == "TextShape.font.wfileName")
                    out.font = f.text();
                else if (f.name == "TextShape.font.pixelSize")
                    size = (float)f.as<int32_t>();
                else if (f.name == "TextShape.font.size")
                    size = f.as<float>();
                else if (f.name == "TextShape.font.glowRadius")
                    radius = (float)f.as<int32_t>() * 0.5f;
                else if (f.name == "TextShape.font.strokeWidth")
                    radius = f.as<float>() * 0.5f;
            }
            out.pixelSize = size, out.strokeRadius = radius;
        }
    }
    return true;
}

} // namespace plasma
