#pragma once
// The interface's shapes as Plasma turns them into triangles (docs/specs/plasma-shapes-and-text.md,
// parts 1 to 3): a SmoothMeshShape's control polygons, the outline loops their free edges form,
// those loops refined into curves, and from them
//   - the fill: a fan per face when not subdivided, the polygons' Catmull-Clark style surface
//     subdivision, or (outline mode) the curves filled with the nonzero rule;
//   - the stroke: a ribbon along each curve, with miter or round joins at sharp points and
//     round or square caps on open curves.
// Also the drawing's colour arithmetic (the engine's shaders, 2.15), which the HUD applies on the
// CPU per vertex where it can.
#include "Plx.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace plasma {

struct Vertex {
    glm::vec2 position{0};
    glm::vec4 color{1};
    glm::vec2 uv{0};
};
struct Triangles {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices; // a triangle list
    std::vector<std::vector<int>> outlines; // Final drawing boundaries, cached after building.
};
// Native drawing containment: inclusive bounds, then even/odd crossings of its index outlines.
bool contains(const Triangles &drawing, glm::vec2 point);

// SmoothMeshShape.flags (spec 1.2).
enum : int { MESH_OUTLINE = 0x1, MESH_STROKE = 0x2, MESH_NO_FILL = 0x4, MESH_OPEN = 0x8, MESH_EXTRUDED = 0x10 };
// The smoothing rule of a control point: the low 16 bits of its vertexFlags (spec 1.3).
enum : int { POINT_SMOOTH = 0, POINT_SHARP = 1, POINT_LINEAR = 2, POINT_WEIGHTED_CORNER = 3 };
// strokeJointType and strokeCapType values (spec 1.1).
enum : int { JOIN_ROUND = 1, CAP_BUTT = 0, CAP_ROUND = 1, CAP_SQUARE = 2 };

// Translate(translation) . Translate(pivot) . RotX . RotY . RotZ . deformation . Translate(-pivot)
// (column vectors, angles in degrees): a point is deformed and rotated about the pivot, then
// moved. Both a transformation's local matrix (format spec 2.2.1) and a texture matrix (2.12)
// are built so.
glm::mat4 pivoted(glm::vec2 translation, glm::vec2 pivot, glm::vec3 degrees, const glm::mat4 &deformation);

// The values of a SmoothMeshShape that shape its triangles: the live values of its attributes,
// the per-vertex arrays sized to the vertex count (spec 1.1).
struct SmoothMesh {
    std::vector<std::vector<int>> faces;
    std::vector<glm::vec2> positions, texCoords;
    std::vector<glm::vec4> colors, strokeColors;
    std::vector<float> strokeWidths; // full widths
    std::vector<int32_t> vertexFlags;
    std::vector<float> vertexParameters;
    int texture = -1, strokeTexture = -1; // Texture.ids, -1 for none
    glm::vec2 textureTranslation{0}, texturePivot{0};
    glm::vec3 textureRotation{0}; // degrees
    glm::mat4 textureDeformation{1};
    float textureOpacity = 1, textureBrightness = 1, textureContrast = 1, textureSaturation = 1;
    float strokeTextureOpacity = 1, strokeTextureBrightness = 1, strokeTextureContrast = 1, strokeTextureSaturation = 1;
    float strokeTextureStretch = 1;
    int subdivisions = 3;
    float smoothWeight = 0.58578644f; // (sqrt 2 - 1) / (sqrt 2 * 0.5)
    int flags = 0;
    int strokeJoint = 0, strokeCap = CAP_BUTT, strokeAlignment = 0, strokePattern = 0;
    float strokeDash = 1, strokeGap = 1;
};
// Reads a SmoothMeshShape object's values. False for another kind of object.
bool smoothMeshOf(const plx::Object &shape, SmoothMesh &out);

// How a drawing samples its texture (spec 2.12, 2.13): which texture, the matrix its texture
// coordinates go through, and the adjustments of the samples.
struct TextureLook {
    int texture = -1;
    glm::mat4 matrix{1};
    float opacity = 1, brightness = 1, contrast = 1, saturation = 1;
};

// What a shape draws: its fill, then its stroke over it.
struct ShapeDrawing {
    bool filled = false, stroked = false;
    Triangles fill, stroke;
    TextureLook fillLook, strokeLook;
    glm::vec2 minimum{0}, maximum{0}; // the bounds (spec 2.14)
};
ShapeDrawing build(const SmoothMesh &mesh);
// Reads and builds a SmoothMeshShape. False for another kind of object.
bool buildSmoothMesh(const plx::Object &shape, ShapeDrawing &out);

// The outline loops of a face list: the chains of its free edges (spec 2.2).
std::vector<std::vector<int>> outlineLoops(const std::vector<std::vector<int>> &faces);

// Fills contours of vertices with the nonzero winding rule (the GLU tessellator, as Plasma
// does): the triangles' indices; vertices made where contours cross are appended.
std::vector<uint32_t> tessellate(std::vector<Vertex> &vertices, const std::vector<std::vector<uint32_t>> &contours);

// The engine's colour arithmetic of one drawn pixel (spec 2.15, pixel shader): a colour after
// its texture sample, and after the clip layer's sample in a mask mode (1 to 3; 0 leaves it).
glm::vec4 textured(glm::vec4 color, glm::vec4 sample, const TextureLook &look);
glm::vec4 masked(glm::vec4 color, glm::vec4 layer, int maskMode);
// The engine's vertex shader moves every drawn point by this much (pixels, both axes).
constexpr float DRAW_OFFSET = -0.135f;

} // namespace plasma
