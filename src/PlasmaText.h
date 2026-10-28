#pragma once
// The interface's text as Plasma draws it (docs/specs/plasma-shapes-and-text.md, parts 4 to 7):
// a TextShape's values; the sizes a draw rasterises at under the current matrix, in the pixel
// mode (the matrix's scale and rotation handed to FreeType, glyphs on whole screen pixels) or the
// scalable mode (glyphs rasterised at the matrix's scale, pens on whole raster pixels); glyphs
// rasterised by FreeType from the owner's font files, outlines by FreeType's stroker; and the
// layout: pen arithmetic, word wrap, measuring and alignment.
#include "Plx.h"

#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace plasma {

// TextShape.flags (spec 4.2).
enum : int { TEXT_CENTER = 0x1, TEXT_RIGHT = 0x2, TEXT_MIDDLE = 0x4, TEXT_BOTTOM = 0x8, TEXT_WRAP = 0x10 };

// A TextShape's values (spec 4.1): the live string and colours.
struct TextStyle {
    std::wstring string;
    std::string font; // the font file's name
    float pixelSize = 0;    // S, pixels per em
    float strokeRadius = 0; // r, the outline's radius (0: no outline)
    float spacing = 0, lineSpacing = 0, wrapWidth = 0;
    int flags = 0;
    bool scalable = false; // the scalable raster mode (documents read with LOAD_SCALABLE_TEXT)
    glm::vec4 color{0, 0, 0, 1}, strokeColor{0, 0, 0, 1}, extrusionColor{0, 0, 0, 1};
};
// Reads a TextShape object of a document read with these loader flags. False for another kind.
bool textStyle(const plx::Object &shape, int loaderFlags, TextStyle &out);

// What one text draw rasterises at under the current matrix (spec 6.2).
struct Raster {
    bool scalable = false;
    glm::vec2 size{1};      // Sx, Sy: the char size FreeType is given (pixels per em)
    glm::mat2 transform{1}; // pixel mode: the matrix's 2x2 part, which FreeType applies
    float stroke = 0;       // the outline's radius in raster pixels
    float spacing = 0, leading = 0, wrapWidth = 0; // in raster pixels
    // What glyphs rasterised at this size share (for caching them).
    std::array<int, 8> key() const;
};
Raster rasterFor(const glm::mat4 &matrix, const TextStyle &style);
// The matrix the glyph quads are drawn through (spec 6.1 step 3): the translation rounded to a
// whole pixel; in the pixel mode nothing else, in the scalable mode the matrix scaled back from
// raster pixels.
glm::mat4 drawMatrix(const glm::mat4 &matrix, const TextStyle &style, const Raster &raster);

// One glyph rasterised (spec 6.4): a texture of coverage with a transparent one-pixel border, the
// quad it is drawn as relative to the pen (raster pixels, y down), and the pen's advance.
struct Glyph {
    bool exists = false;
    glm::vec2 offset{0}, extent{0};
    glm::ivec2 size{0}; // the texture's pixels
    std::vector<uint8_t> alpha; // size.x * size.y, row by row
    float advance = 0;
};

// A font file opened with FreeType; glyphs are kept once rasterised.
class Font {
  public:
    Font();
    ~Font();
    Font(const Font &) = delete;
    Font &operator=(const Font &) = delete;
    bool load(std::vector<unsigned char> data);
    bool loaded() const;
    // The plain glyph of a character, or its outline (the full stroke of radius raster.stroke).
    const Glyph &glyph(wchar_t c, const Raster &raster, bool outline = false) const;
    // FreeType's kerning of a pair (0 for fonts without a kern table, such as the game's).
    float kerning(wchar_t a, wchar_t b, const Raster &raster) const;

  private:
    struct Face;
    std::unique_ptr<Face> face;
};

// The pen after one character (spec 6.5): its advance and, unless it is the last character, the
// kerning with the next one and the spacing; cut to whole raster pixels in the scalable mode.
float nextPen(float pen, float advance, float kerning, float spacing, bool last, bool scalable);

// A text's box (spec 6.7), in raster pixels from the origin: of the pens ("pen" mode, one place past
// the end included) or of the glyphs' ink ("box" mode). `start` -1 measures the whole text, any
// other index the line from there. Aligned by the flags.
struct TextBox {
    glm::vec2 minimum{0}, maximum{0};
};
TextBox measure(const Font &font, const std::wstring &text, const Raster &raster, int flags, int start, bool inkBox);
// The text with spaces turned into line breaks to fit the raster's wrap width (spec 6.6).
std::wstring wrapText(const Font &font, std::wstring text, const Raster &raster);

// A laid out text (spec 6.8): each character's pen in raster pixels from the origin, and the final
// pen. The quad of a glyph at a pen goes at glyphPosition().
struct Placed {
    wchar_t c;
    glm::vec2 pen;
};
struct Layout {
    std::vector<Placed> glyphs;
    glm::vec2 pen{0};
};
Layout layout(const Font &font, const std::wstring &text, const Raster &raster, int flags);
// Where a glyph's quad starts (whole pixels of the draw matrix): its pen through the layout
// matrix (the pixel mode's 2x2 part; none in the scalable mode) plus the glyph's offset.
glm::vec2 glyphPosition(const Raster &raster, glm::vec2 pen, glm::vec2 glyphOffset);

// A TextShape's bounds (spec 6.7): the ink box of its text, in the shape's units. Only scalable
// texts have bounds; false otherwise.
bool textBounds(const Font &font, const TextStyle &style, const glm::mat4 &matrix, TextBox &out);

} // namespace plasma
